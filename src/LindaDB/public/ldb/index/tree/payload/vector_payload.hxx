/* LindaDB project
 *
 * Copyright (c) 2023 András Bodor <bodand@pm.me>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the copyright holder nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Originally created: 2023-10-14.
 *
 * src/LindaDB/public/ldb/index/tree/payload/vector_payload --
 *   The most-basic multi-storage tree payload. Stores an array of kv-pairs, into
 *   which it copies/moves the different keys and values.
 *   The array is sorted and thusly lookup is done through binary search.
 */
#ifndef LINDADB_VECTOR_PAYLOAD_HXX
#define LINDADB_VECTOR_PAYLOAD_HXX

#include <array>
#include <cassert>
#include <concepts>
#include <utility>

#include <ldb/index/tree/payload.hxx>
#include <ldb/profiler.hxx>

namespace ldb::index::tree::payloads {
    template<std::movable K, std::movable V, std::size_t Clustering>
    struct vector_payload final {
        using key_type = K;
        using value_type = V;
        using size_type = std::size_t;

        constexpr vector_payload() = default;

        template<class K2 = K, class V2 = V>
        constexpr vector_payload(K2&& key, V2&& value)
             : _data_sz(1),
               _data({std::make_pair(std::forward<K2>(key), std::forward<V2>(value))}) { }

        vector_payload(const vector_payload& cp)
            requires(std::copyable<std::pair<K, V>>)
        = default;
        vector_payload(vector_payload&& mv) noexcept = default;
        vector_payload&
        operator=(const vector_payload& cp)
            requires(std::copyable<std::pair<K, V>>)
        = default;
        vector_payload&
        operator=(vector_payload&& mv) noexcept = default;

        ~vector_payload() noexcept = default;

        [[nodiscard]] LDB_CONSTEXPR23 std::weak_ordering
        operator<=>(const K& key) const noexcept {
            LDB_PROF_SCOPE("VectorPayload_Ordering");
            if (empty()) return std::weak_ordering::equivalent;
            auto mem_min_key = min_key();
            auto mem_max_key = max_key();
            if (key < mem_min_key) return std::weak_ordering::greater;
            if (mem_max_key < key) return std::weak_ordering::less;
            return std::weak_ordering::equivalent;
        }

        [[nodiscard]] std::weak_ordering
        operator<=>(const K& key) const noexcept
            requires(std::same_as<K, std::string>)
        {
            LDB_PROF_SCOPE("VectorPayload_StrOrdering");
            if (empty()) return std::weak_ordering::equivalent;
            if (key.empty()) return std::weak_ordering::less;
            if (key[0] < min_key()[0]) return std::weak_ordering::greater;
            if (key[0] > max_key()[0]) return std::weak_ordering::less;
            if (key < min_key()) return std::weak_ordering::greater;
            if (key > max_key()) return std::weak_ordering::less;
            return std::weak_ordering::equivalent;
        }

        [[nodiscard]] constexpr size_type
        capacity() const noexcept { return Clustering; }

        [[nodiscard]] constexpr size_type
        size() const noexcept { return _data_sz; }

        [[nodiscard]] constexpr bool
        full() const noexcept { return _data_sz == Clustering; }

        [[nodiscard]] constexpr bool
        empty() const noexcept { return _data_sz == 0; }

        [[nodiscard]] constexpr bool
        have_priority() const noexcept { return _data_sz < 2; }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<V>
        try_get(const K& key) const noexcept(std::is_nothrow_constructible_v<std::optional<V>, V>) {
            LDB_PROF_SCOPE_C("VectorPayload_Search", prof::color_search);
            if (empty()) return std::nullopt;
            auto data_end_offset = static_cast<std::ptrdiff_t>(_data_sz);
            if (auto it = std::lower_bound(std::begin(_data),
                                           std::next(std::begin(_data), data_end_offset),
                                           key,
                                           compare_pair_to_key);
                it != std::next(std::begin(_data), data_end_offset)) {
                return {it->second};
            }
            return std::nullopt;
        }

        [[nodiscard]] bool
        try_set(const K& key, const V& value) {
            LDB_PROF_SCOPE_C("VectorPayload_Insert", ldb::prof::color_insert);
            return upsert_kv(true, key, value) & (INSERTED | UPDATED);
        }

        [[nodiscard]] std::optional<std::pair<K, V>>
        force_set(const K& key, const V& value) {
            LDB_PROF_SCOPE_C("VectorPayload_InsertAndSquish", ldb::prof::color_insert);
            if (auto res = upsert_kv(true, key, value);
                res == FULL) {
                auto squished = _data[0];
                std::move(std::next(std::begin(_data)),
                          std::next(std::begin(_data), static_cast<std::ptrdiff_t>(_data_sz)),
                          std::begin(_data));
                res = upsert_kv(false, key, value);
                assert(res != FAILURE);
                return {squished};
            }
            return std::nullopt;
        }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const vector_payload& pl) {
            os << "(" << pl.capacity() << " " << pl.size();
            for (std::size_t i = 0; i < pl._data_sz; ++i) {
                os << " (" << pl._data[i].first << " " << pl._data[i].second << ")";
            }
            return os << ")";
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator==(const vector_payload<K, V, Clustering>& self, const K& other) noexcept(noexcept(self <=> other)) {
            LDB_PROF_SCOPE("VectorPayload_LeftEq");
            return (self <=> other) == 0;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator==(const K& other, const vector_payload<K, V, Clustering>& self) noexcept(noexcept(other <=> self)) {
            LDB_PROF_SCOPE("VectorPayload_RightEq");
            return (other <=> self) == 0;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator!=(const vector_payload<K, V, Clustering>& self, const K& other) noexcept(noexcept(other <=> self)) {
            LDB_PROF_SCOPE("VectorPayload_LeftNe");
            return (self <=> other) != 0;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator!=(const K& other, const vector_payload<K, V, Clustering>& self) noexcept(noexcept(other <=> self)) {
            LDB_PROF_SCOPE("VectorPayload_RightNe");
            return (other <=> self) != 0;
        }

        [[nodiscard]] constexpr const K&
        min_key() const noexcept {
            assert(!empty() && "min_key of empty vector_payload");
            return _data[0].first;
        }

        [[nodiscard]] constexpr const K&
        max_key() const noexcept {
            assert(!empty() && "max_key of empty vector_payload");
            return _data[_data_sz - 1].first;
        }

        constexpr const static auto compare_pair_to_key =
               [](const std::pair<K, V>& store, const K& key_cmp) noexcept(noexcept(store.first < key_cmp)) {
                   return store.first < key_cmp;
               };

        enum upsert_status : unsigned {
            UPDATED = 1U,
            INSERTED = 1U << 1U,
            FAILURE = 1U << 2U,
            FULL = 1U << 3U
        };

        upsert_status
        upsert_kv(bool do_upsert, const K& key, const V& value) {
            LDB_PROF_SCOPE_C("VectorPayload_Upsert", prof::color_insert);
            if (_data_sz < 2) { // with 0 or 1 elems insertion is trivial
                if (_data[0].first == key) {
                    if (!do_upsert) return FAILURE;
                    _data[0].second = value;
                    return UPDATED;
                }
                _data[_data_sz++] = std::make_pair(key, value);
                if (_data[0].first > key) std::swap(_data[0], _data[1]);

                return INSERTED;
            }

            auto data_end_offset = static_cast<std::ptrdiff_t>(_data_sz);
            auto it = std::lower_bound(std::begin(_data),
                                       std::next(std::begin(_data), data_end_offset),
                                       key,
                                       compare_pair_to_key);
            if (it == std::end(_data)) return FULL;
            if (it->first == key) {
                if (!do_upsert) return FAILURE;
                it->second = value;
                return UPDATED;
            }
            if (full()) return FULL;

            std::move_backward(it, std::next(std::begin(_data), data_end_offset), std::next(std::begin(_data), data_end_offset + 1));
            *it = std::make_pair(key, value);
            ++_data_sz;
            return INSERTED;
        }

        std::size_t _data_sz{0};
        std::array<std::pair<K, V>, Clustering> _data{};
    };
    static_assert(payload<vector_payload<int, int, 0>>);
}

#endif