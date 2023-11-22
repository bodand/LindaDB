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
 * Originally created: 2023-10-28.
 *
 * src/LindaDB/public/ldb/index/tree/payload/chime_payload --
 *   A payload type for implementing a chime-tree. A chime-tree is a special type
 *   of T-tree that allows multiple values to be assigned to a key.
 */
#ifndef LINDADB_CHIME_PAYLOAD_HXX
#define LINDADB_CHIME_PAYLOAD_HXX

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <shared_mutex>
#include <utility>
#include <vector>

#include <ldb/index/tree/index_query.hxx>
#include <ldb/index/tree/payload.hxx>
#include <ldb/profiler.hxx>

namespace ldb::index::tree::payloads {
    template<std::movable K, std::movable V, std::size_t Clustering>
    struct chime_payload final {
        using key_type = K;
        using value_type = V;
        using size_type = std::size_t;

        struct bundle_type {
            key_type key;
            std::vector<value_type> data;
        };

        constexpr
        chime_payload() = default;

        template<class K2 = key_type, class V2 = value_type>
        constexpr chime_payload(K2&& key, V2&& value)
             : _data_sz(1),
               _keys{std::forward<K2>(key)},
               _sets{chime_value_set(std::forward<V2>(value))} { }

        constexpr explicit
        chime_payload(bundle_type&& bundle)
             : _data_sz(1),
               _keys{std::move(bundle.key)},
               _sets{chime_value_set(std::move(bundle))} { }

        constexpr explicit
        chime_payload(const bundle_type& bundle)
             : _data_sz(1),
               _keys{bundle.key},
               _sets{chime_value_set(bundle)} { }

        constexpr
        chime_payload(const chime_payload& cp)
            requires(std::copyable<key_type> && std::copyable<value_type>)
        = default;

        constexpr
        chime_payload(chime_payload&& mv) noexcept = default;
        constexpr chime_payload&
        operator=(const chime_payload& cp)
            requires(std::copyable<key_type> && std::copyable<value_type>)
        = default;

        constexpr chime_payload&
        operator=(chime_payload&& mv) noexcept = default;

        constexpr ~
        chime_payload() noexcept = default;

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


        [[nodiscard]] LDB_CONSTEXPR23 std::weak_ordering
        operator<=>(const auto& key) const noexcept {
            LDB_PROF_SCOPE("ChimePayload_Ordering");
            if (empty()) return std::weak_ordering::equivalent;
            auto mem_min_key = min_key();
            auto mem_max_key = max_key();
            if (key < mem_min_key) return std::weak_ordering::greater;
            if (mem_max_key < key) return std::weak_ordering::less;
            return std::weak_ordering::equivalent;
        }

        bool
        try_merge(chime_payload& other) {
            if (capacity() - size() < other.size()) return false;
            // todo: proper merge algorithm
            for (std::size_t i = 0; i < other.size(); ++i) {
                auto succ = try_set(bundle_type{
                       .key = other._keys[i],
                       .data = other._sets[i].flush()});
                assert(succ);
            }
            other._data_sz = 0;
            return true;
        }

        void
        merge_until_full(chime_payload& other) {
            if (size() == capacity()) return;
            // todo: proper merge algorithm
            for (std::size_t i = 0; i < other.size(); ++i) {
                auto succ = try_set(bundle_type{
                       .key = other._keys[i],
                       .data = other._sets[i].flush()});
                if (!succ) break;
                --other._data_sz;
            }
        }

        [[nodiscard]] bool
        try_set(const key_type& key, const value_type& value) {
            LDB_PROF_SCOPE_C("ChimePayload_Insert", prof::color_insert);
            return upsert_kv(key, {&value, 1}) & (INSERTED | UPDATED);
        }

        [[nodiscard]] bool
        try_set(const bundle_type& bundle) {
            LDB_PROF_SCOPE_C("ChimePayload_Insert", prof::color_insert);
            return upsert_kv(bundle.key, bundle.data) & (INSERTED | UPDATED);
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_lower(const key_type& key, const value_type& value) {
            LDB_PROF_SCOPE_C("ChimePayload_InsertAndSquishL", prof::color_insert);
            if (auto res = upsert_kv(key, {&value, 1});
                res == FULL) {
                auto squished = bundle_type{.key = _keys[0], .data = _sets[0].flush()};
                std::move(std::next(std::begin(_keys)),
                          std::next(std::begin(_keys), static_cast<std::ptrdiff_t>(_data_sz)),
                          std::begin(_keys));
                std::move(std::next(std::begin(_sets)),
                          std::next(std::begin(_sets), static_cast<std::ptrdiff_t>(_data_sz)),
                          std::begin(_sets));
                --_data_sz;
                res = upsert_kv(key, {&value, 1});
                return {squished};
            }
            return std::nullopt;
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_upper(const key_type& key, const value_type& value) {
            LDB_PROF_SCOPE_C("ChimePayload_InsertAndSquishU", prof::color_insert);
            if (auto res = upsert_kv(key, {&value, 1});
                res == FULL) {
                auto squished = bundle_type{.key = _keys.back(), .data = _sets.back().flush()};
                --_data_sz;
                res = upsert_kv(key, {&value, 1});
                return {squished};
            }
            return std::nullopt;
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_lower(bundle_type&& bundle) {
            LDB_PROF_SCOPE_C("ChimePayload_InsertAndSquishL", prof::color_insert);
            auto [key, data] = std::move(bundle);
            if (auto res = upsert_kv(key, data);
                res == FULL) {
                auto squished = bundle_type{.key = _keys[0], .data = _sets[0]};
                std::move(std::next(std::begin(_keys)),
                          std::next(std::begin(_keys), static_cast<std::ptrdiff_t>(_data_sz)),
                          std::begin(_keys));
                std::move(std::next(std::begin(_sets)),
                          std::next(std::begin(_sets), static_cast<std::ptrdiff_t>(_data_sz)),
                          std::begin(_sets));
                --_data_sz;
                res = upsert_kv(std::move(key), std::move(data));
                return {squished};
            }
            return std::nullopt;
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_upper(bundle_type&& bundle) {
            LDB_PROF_SCOPE_C("ChimePayload_InsertAndSquishU", prof::color_insert);
            auto [key, data] = std::move(bundle);
            if (auto res = upsert_kv(key, data);
                res == FULL) {
                auto squished = bundle_type{.key = _keys.back(), .data = _sets.back().flush()};
                --_data_sz;
                res = upsert_kv(std::move(key), std::move(data));
                return {squished};
            }
            return std::nullopt;
        }

        template<index_query<value_type> Q>
        [[nodiscard]] LDB_CONSTEXPR23 std::optional<value_type>
        try_get(const Q& query) const noexcept(std::is_nothrow_constructible_v<std::optional<value_type>, value_type>) {
            LDB_PROF_SCOPE_C("ChimePayload_Search", prof::color_search);
            if (empty()) return std::nullopt;
            auto data_end = std::next(begin(_keys), static_cast<std::ptrdiff_t>(_data_sz));
            if (auto it = std::lower_bound(begin(_keys),
                                           data_end,
                                           query.key());
                it != data_end) {
                const auto col_idx = static_cast<std::size_t>(std::distance(begin(_keys), it));
                return _sets[col_idx].get(query);
            }
            return std::nullopt;
        }

        template<index_query<value_type> Q>
        LDB_CONSTEXPR23 std::optional<value_type>
        remove(const Q& query) {
            LDB_PROF_SCOPE_C("ChimePayload_Remove", prof::color_remove);
            if (empty()) return std::nullopt;
            auto key_end = std::next(begin(_keys), static_cast<std::ptrdiff_t>(_data_sz));
            auto it = std::lower_bound(begin(_keys), key_end, query.key());
            if (it == key_end) return std::nullopt;

            const auto col_idx = static_cast<size_type>(std::distance(begin(_keys), it));
            auto res = _sets[col_idx].pop(query);

            if (_sets[col_idx].empty()) {
                const auto data_it = std::next(begin(_sets), static_cast<std::ptrdiff_t>(col_idx));
                const auto data_end = std::next(begin(_sets), static_cast<std::ptrdiff_t>(_data_sz));
                std::ranges::rotate(it, it + 1, key_end);
                std::ranges::rotate(data_it, data_it + 1, data_end);
                --_data_sz;
            }
            return res;
        }

    private:
        struct chime_value_set {
            constexpr
            chime_value_set() = default;

            constexpr explicit
            chime_value_set(bundle_type&& bundle)
                 : _values(std::move(bundle).data) {
                assert(std::ranges::is_sorted(_values) && "chime_value_set must be sorted when constructed from bundle");
            }

            constexpr explicit
            chime_value_set(const bundle_type& bundle)
                 : _values(bundle.data) {
                assert(std::ranges::is_sorted(_values) && "chime_value_set must be sorted when constructed from bundle");
            }

            template<class SetV>
            constexpr explicit chime_value_set(SetV&& value) // NOLINT(*-forwarding-reference-overload)
                requires(!std::same_as<SetV, chime_value_set>)
                 : _values{std::forward<SetV>(value)} { }

            LDB_CONSTEXPR23 void
            push(std::span<const value_type> val) {
                LDB_PROF_SCOPE("ChimeValueSet_Push");
                _values.reserve(_values.size() + val.size());
                std::ranges::for_each(val, [&](const auto& item) {
                    auto it = std::ranges::lower_bound(_values, item);
                    _values.insert(it, item);
                });
            }

            template<index_query<value_type> Q>
            [[nodiscard]] LDB_CONSTEXPR23 std::optional<value_type>
            pop(Q query) {
                LDB_PROF_SCOPE("ChimeValueSet_Push");
                assert(!empty());
                if (auto it = std::lower_bound(_values.begin(), _values.end(), query);
                    it != _values.end()) {

                    auto cp = *it;
                    _values.erase(it);
                    return cp;
                }
                return std::nullopt;
            }

            template<index_query<value_type> Q>
            [[nodiscard]] LDB_CONSTEXPR23 std::optional<value_type>
            get(const Q& query) const {
                LDB_PROF_SCOPE("ChimeValueSet_Get");
                assert(!empty());
                if (auto it = std::lower_bound(_values.begin(), _values.end(), query);
                    it != _values.end()) return *it;
                return std::nullopt;
            }

            [[nodiscard]] LDB_CONSTEXPR23 bool
            check(const value_type& val) const {
                LDB_PROF_SCOPE("ChimeValueSet_Check");
                return std::ranges::binary_search(_values, val);
            }

            [[nodiscard]] constexpr bool
            empty() const {
                return _values.empty();
            }

            LDB_CONSTEXPR23 std::vector<value_type>
            flush() {
                LDB_PROF_SCOPE("ChimeValueSet_Flush");
                auto res = std::move(_values);
                _values = std::vector<value_type>();
                return res;
            }

        private:
            friend constexpr std::ostream&
            operator<<(std::ostream& os, const chime_value_set& self) {
                os << "(";
                for (std::size_t i = 0; i < self._values.size(); ++i) {
                    os << self._values[i];
                    if (i != self._values.size() - 1) os << " ";
                }
                return os << ")";
            }

            std::vector<value_type> _values{};
        };

        friend constexpr std::ostream&
        operator<<(std::ostream& os, const chime_payload& pl) {
            os << "(" << pl.capacity() << " " << pl.size();
            for (std::size_t i = 0; i < pl._data_sz; ++i) {
                os << " (" << pl._keys[i] << " " << pl._sets[i] << ")";
            }
            return os << ")";
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator==(const chime_payload<key_type, value_type, Clustering>& self, const key_type& other) noexcept(noexcept(self <=> other)) {
            LDB_PROF_SCOPE("ChimePayload_LeftEq");
            return (self <=> other) == 0;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator==(const key_type& other, const chime_payload<key_type, value_type, Clustering>& self) noexcept(noexcept(other <=> self)) {
            LDB_PROF_SCOPE("ChimePayload_RightEq");
            return (other <=> self) == 0;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator!=(const chime_payload<key_type, value_type, Clustering>& self, const key_type& other) noexcept(noexcept(other <=> self)) {
            LDB_PROF_SCOPE("ChimePayload_LeftNe");
            return (self <=> other) != 0;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator!=(const key_type& other, const chime_payload<key_type, value_type, Clustering>& self) noexcept(noexcept(other <=> self)) {
            LDB_PROF_SCOPE("ChimePayload_RightNe");
            return (other <=> self) != 0;
        }

        [[nodiscard]] constexpr const key_type&
        min_key() const noexcept {
            assert(!empty() && "min_key of empty chime_payload");
            return _keys[0];
        }

        [[nodiscard]] constexpr const key_type&
        max_key() const noexcept {
            assert(!empty() && "max_key of empty chime_payload");
            return _keys[_data_sz - 1];
        }

        enum upsert_status : unsigned {
            UPDATED = 1U,
            INSERTED = 1U << 1U,
            FULL = 1U << 2U
        };

        LDB_CONSTEXPR23 std::optional<value_type>
        drop(const key_type& key) {
            LDB_PROF_SCOPE_C("ChimePayload_Drop", prof::color_remove);
            const auto data_end = next(begin(_keys), _data_sz);
            const auto it = std::ranges::lower_bound(begin(_keys),
                                                     data_end,
                                                     key);
            if (it == data_end) return std::nullopt;
            const auto col_idx = distance(begin(_keys), it);
            const auto res = _sets[col_idx].pop();
            if (_sets[col_idx].empty()) {
                // remove key-set
                auto sets_it = next(begin(_sets), col_idx);
                auto sets_end = next(begin(_sets), _data_sz);
                std::ranges::rotate(sets_it, sets_it + 1, sets_end);
                std::ranges::rotate(it, it + 1, data_end);
                std::ignore = _sets[--_data_sz].flush();
            }
            return res;
        }

        LDB_CONSTEXPR23 upsert_status
        upsert_kv(const key_type& key, std::span<const value_type> value) {
            LDB_PROF_SCOPE_C("ChimePayload_Upsert", prof::color_insert);
            using std::swap;
            if (_data_sz < 2) { // with 0 or 1 elems insertion is trivial
                if (_data_sz > 0 && _keys[0] == key) {
                    _sets[0].push(value);
                    return UPDATED;
                }
                _sets[_data_sz].push(value);
                _keys[_data_sz++] = key;
                if (_data_sz > 1 && _keys[0] > key) {
                    swap(_keys[0], _keys[1]);
                    swap(_sets[0], _sets[1]);
                }

                return INSERTED;
            }

            auto data_end_offset = static_cast<std::ptrdiff_t>(_data_sz);
            auto it = std::ranges::lower_bound(begin(_keys),
                                               std::next(begin(_keys), data_end_offset),
                                               key);
            if (it == end(_keys)) return FULL;

            auto col_idx = static_cast<std::size_t>(std::distance(begin(_keys), it));
            if (*it == key) {
                _sets[col_idx].push(value);
                return UPDATED;
            }
            if (full()) return FULL;

            std::ranges::move_backward(it,
                                       std::next(begin(_keys), data_end_offset),
                                       std::next(begin(_keys), data_end_offset + 1));
            std::ranges::move_backward(std::next(begin(_sets), static_cast<std::ptrdiff_t>(col_idx)),
                                       std::next(begin(_sets), data_end_offset),
                                       std::next(begin(_sets), data_end_offset + 1));
            *it = key;
            _sets[col_idx].push(value);
            ++_data_sz;
            return INSERTED;
        }

        std::size_t _data_sz{0};
        std::array<key_type, Clustering> _keys{};
        std::array<chime_value_set, Clustering> _sets{};
    };
    static_assert(payload<chime_payload<int, int, 0>>);
}

#endif
