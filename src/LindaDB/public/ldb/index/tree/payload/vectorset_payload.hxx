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
 * src/LindaDB/public/ldb/index/tree/payload/vectorset_payload --
 *   A vector-based T-tree payload that implements a set-like behavior.
 *   There are no keys, the only value stored is the one used for lookups.
 *   The array is sorted and thusly lookup is done through binary search.
 */
#ifndef LINDADB_vectorset_PAYLOAD_HXX
#define LINDADB_vectorset_PAYLOAD_HXX

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstdlib>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include <ldb/common.hxx>
#include <ldb/index/tree/payload.hxx>

namespace ldb::index::tree::payloads {
    template<std::movable V, std::size_t Clustering>
    struct vectorset_payload final {
        using key_type = V;
        using value_type = V;
        using size_type = std::size_t;
        using bundle_type = value_type;

        constexpr vectorset_payload() = default;

        template<class K = key_type, class V2 = value_type>
        constexpr vectorset_payload(K&&, V2&& value)
             : _data_sz(1),
               _data({std::forward<V2>(value)}) {
            LDBT_ZONE_A;
        }

        constexpr explicit vectorset_payload(bundle_type&& bundle)
             : _data_sz(1),
               _data({std::move(bundle)}) {
            LDBT_ZONE_A;
        }

        constexpr explicit vectorset_payload(const bundle_type& bundle)
             : _data_sz(1),
               _data({bundle}) {
            LDBT_ZONE_A;
        }

        constexpr vectorset_payload(const vectorset_payload& cp)
            requires(std::copyable<value_type>)
        = default;
        constexpr vectorset_payload(vectorset_payload&& mv) noexcept = default;
        constexpr vectorset_payload&
        operator=(const vectorset_payload& cp)
            requires(std::copyable<value_type>)
        = default;
        constexpr vectorset_payload&
        operator=(vectorset_payload&& mv) noexcept = default;

        constexpr ~vectorset_payload() noexcept = default;

        [[nodiscard]] constexpr std::weak_ordering
        operator<=>(const auto& key) const noexcept {
            LDBT_ZONE_A;
            if (empty()) return std::weak_ordering::equivalent;
            auto mem_min_key = min_key();
            auto mem_max_key = max_key();
            if (key < mem_min_key) return std::weak_ordering::greater;
            if (mem_max_key < key) return std::weak_ordering::less;
            return std::weak_ordering::equivalent;
        }

        [[nodiscard]] constexpr size_type
        capacity() const noexcept {
            LDBT_ZONE_A;
            return Clustering;
        }

        [[nodiscard]] constexpr size_type
        size() const noexcept {
            LDBT_ZONE_A;
            return _data_sz;
        }

        [[nodiscard]] constexpr bool
        full() const noexcept {
            LDBT_ZONE_A;
            return _data_sz == Clustering;
        }

        [[nodiscard]] constexpr bool
        empty() const noexcept {
            LDBT_ZONE_A;
            return _data_sz == 0;
        }

        [[nodiscard]] constexpr bool
        have_priority() const noexcept {
            LDBT_ZONE_A;
            return _data_sz < 2;
        }

        bool
        try_merge(vectorset_payload& other) {
            LDBT_ZONE_A;
            if (capacity() - size() < other.size()) return false;
            // todo: proper merge algorithm
            for (std::size_t i = 0; i < other.size(); ++i) {
                auto succ = try_set(other._data[i]);
                assert_that(succ);
            }
            other._data_sz = 0;
            return true;
        }

        void
        merge_until_full(vectorset_payload& other) {
            LDBT_ZONE_A;
            if (size() == capacity()) return;
            // todo: proper merge algorithm
            for (std::size_t i = 0; i < other.size(); ++i) {
                auto succ = try_set(other._data[i]);
                if (!succ) break;
                --other._data_sz;
            }
        }

        template<index_lookup<value_type> Q>
        [[nodiscard]] constexpr std::optional<value_type>
        try_get(Q query) const noexcept(std::is_nothrow_constructible_v<std::optional<value_type>, value_type>) {
            LDBT_ZONE_A;
            if (empty()) return std::nullopt;
            auto data_end_offset = static_cast<std::ptrdiff_t>(_data_sz);
            if (auto it = std::lower_bound(std::begin(_data),
                                           std::next(std::begin(_data), data_end_offset),
                                           query.key());
                it != std::next(std::begin(_data), data_end_offset)
                && *it == query) {
                return {*it};
            }
            return std::nullopt;
        }

        template<class Q>
        [[nodiscard]] constexpr std::optional<value_type>
        try_get(Q raw_query) const noexcept(std::is_nothrow_constructible_v<std::optional<value_type>, value_type>) {
            LDBT_ZONE_A;
            if (empty()) return std::nullopt;
            auto data_end_offset = static_cast<std::ptrdiff_t>(_data_sz);
            if (auto it = std::lower_bound(std::begin(_data),
                                           std::next(std::begin(_data), data_end_offset),
                                           raw_query,
                                           compare_pair_to_key);
                it != std::next(std::begin(_data), data_end_offset)
                && *it == raw_query) {
                return {*it};
            }
            return std::nullopt;
        }

        [[nodiscard]] constexpr bool
        try_set(const key_type& key, const value_type& value) {
            std::ignore = value;
            LDBT_ZONE_A;
            return upsert_kv(true, key) & (INSERTED | UPDATED);
        }

        [[nodiscard]] constexpr bool
        try_set(const bundle_type& bundle) {
            LDBT_ZONE_A;
            return try_set(bundle, bundle); // convert value to pseudo-kv pair
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_lower(const key_type& key, const value_type& value) {
            std::ignore = value;
            LDBT_ZONE_A;
            if (auto res = upsert_kv(true, key);
                res == FULL) {
                auto squished = _data[0];
                std::move(std::next(std::begin(_data)),
                          std::next(std::begin(_data), static_cast<std::ptrdiff_t>(_data_sz)),
                          std::begin(_data));
                --_data_sz;
                res = upsert_kv(false, key);
                assert_that(res != FAILURE);
                return {squished};
            }
            return std::nullopt;
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_upper(const key_type& key, const value_type& value) {
            std::ignore = value;
            LDBT_ZONE_A;
            if (auto res = upsert_kv(true, key);
                res == FULL) {
                auto squished = _data.back();
                --_data_sz;
                res = upsert_kv(false, key);
                assert_that(res != FAILURE);
                return {squished};
            }
            return std::nullopt;
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_lower(bundle_type&& bundle) {
            LDBT_ZONE_A;
            auto&& [key, value] = std::move(bundle);
            return force_set_lower(std::move(key), std::move(value));
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_upper(bundle_type&& bundle) {
            LDBT_ZONE_A;
            auto&& [key, value] = std::move(bundle);
            return force_set_upper(std::move(key), std::move(value));
        }

        template<index_lookup<value_type> Q>
        constexpr std::optional<value_type>
        remove(Q query) {
            LDBT_ZONE_A;
            assert_that(!empty());
            auto data_end = std::next(begin(_data), _data_sz);
            auto it = std::ranges::lower_bound(begin(_data), data_end, query.key());
            if (it == data_end) return std::nullopt;
            if (*it != query) return std::nullopt;
            std::optional<value_type> ret{*it};
            std::ranges::rotate(it, it + 1, data_end);
            --_data_sz;
            return ret;
        }

        template<class Q>
        constexpr std::optional<value_type>
        remove(Q raw_query) {
            LDBT_ZONE_A;
            if (empty()) return {};
            auto data_end = std::next(begin(_data), static_cast<long long>(_data_sz));
            auto it = std::lower_bound(begin(_data), data_end, raw_query);
            if (it == data_end) return std::nullopt;
            if (*it != raw_query) return std::nullopt;
            std::optional<value_type> ret{*it};
            std::ranges::rotate(it, it + 1, data_end);
            --_data_sz;
            return ret;
        }

        template<class Fn>
        void
        apply(Fn&& fn) {
            LDBT_ZONE_A;
            std::ranges::for_each(_data, std::forward<Fn>(fn), [](const auto& p) { return p.second; });
        }


    private:
        friend constexpr std::ostream&
        operator<<(std::ostream& os, const vectorset_payload& pl) {
            os << "(vector: " << pl.capacity() << " " << pl.size();
            for (std::size_t i = 0; i < pl._data_sz; ++i) {
                os << " (" << pl._data[i] << ")";
            }
            return os << ")";
        }

        [[nodiscard]] friend constexpr bool
        operator==(const vectorset_payload<value_type, Clustering>& self, const value_type& other) noexcept(noexcept(self <=> other)) {
            LDBT_ZONE_A;
            return (self <=> other) == 0;
        }

        [[nodiscard]] friend constexpr bool
        operator==(const value_type& other, const vectorset_payload<value_type, Clustering>& self) noexcept(noexcept(other <=> self)) {
            LDBT_ZONE_A;
            return (other <=> self) == 0;
        }

        [[nodiscard]] friend constexpr bool
        operator!=(const vectorset_payload<value_type, Clustering>& self, const value_type& other) noexcept(noexcept(other <=> self)) {
            LDBT_ZONE_A;
            return (self <=> other) != 0;
        }

        [[nodiscard]] friend constexpr bool
        operator!=(const value_type& other, const vectorset_payload<value_type, Clustering>& self) noexcept(noexcept(other <=> self)) {
            LDBT_ZONE_A;
            return (other <=> self) != 0;
        }

        [[nodiscard]] constexpr const key_type&
        min_key() const noexcept {
            LDBT_ZONE_A;
            assert_that(!empty() && "min_key of empty vectorset_payload");
            return _data[0];
        }

        [[nodiscard]] constexpr const key_type&
        max_key() const noexcept {
            LDBT_ZONE_A;
            assert_that(!empty() && "max_key of empty vectorset_payload");
            return _data[_data_sz - 1];
        }

        constexpr const static auto compare_pair_to_key =
               [](const value_type& store, const auto& key_cmp) noexcept(noexcept(store < key_cmp)) {
                   LDBT_ZONE_A;
                   return store < key_cmp;
               };

        enum upsert_status : unsigned {
            UPDATED = 1U,
            INSERTED = 1U << 1U,
            FAILURE = 1U << 2U,
            FULL = 1U << 3U
        };

        constexpr upsert_status
        upsert_kv(bool do_upsert, const value_type& value) {
            LDBT_ZONE_A;
            if (_data_sz < 2) { // with 0 or 1 elems insertion is trivial
                if (_data[0] == value) {
                    if (!do_upsert) return FAILURE;
                    _data[0] = value;
                    return UPDATED;
                }
                _data[_data_sz++] = value;
                if (_data[0] > value) std::swap(_data[0], _data[1]);

                return INSERTED;
            }

            auto data_end_offset = static_cast<std::ptrdiff_t>(_data_sz);
            auto it = std::lower_bound(std::begin(_data),
                                       std::next(begin(_data), data_end_offset),
                                       value,
                                       compare_pair_to_key);
            if (it == end(_data)) return FULL;
            if (*it == value) {
                if (!do_upsert) return FAILURE;
                *it = value;
                return UPDATED;
            }
            if (full()) return FULL;

            std::ranges::move_backward(it,
                                       std::next(begin(_data), data_end_offset),
                                       std::next(begin(_data), data_end_offset + 1));
            *it = value;
            ++_data_sz;
            return INSERTED;
        }

        std::size_t _data_sz{0};
        std::array<value_type, Clustering> _data{};
    };
    static_assert(payload<vectorset_payload<int, 0>>);
}

#endif
