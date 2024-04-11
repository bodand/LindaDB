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
 * src/LindaDB/public/ldb/index/tree/payload/scalar_payload --
 *   Tree payload type for implementing basic AVL-trees. Only ever stores a scalar
 *   value in itself as a key-value pair.
 *   Because of this behavior it is better suited for tree-s that only have a
 *   clustering of 1: by knowing it always stores a single element, it does not
 *   need to have the trickier bookkeeping required for other multi-element
 *   payloads, allowing better reasoning about code and maybe even performance,
 *   as less code is executed.
 */
#ifndef LINDADB_SCALAR_PAYLOAD_HXX
#define LINDADB_SCALAR_PAYLOAD_HXX

#include <cassert>
#include <compare>
#include <concepts>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <ldb/common.hxx>
#include <ldb/index/tree/index_query.hxx>
#include <ldb/index/tree/payload.hxx>

namespace ldb::index::tree::payloads {
    template<class K, std::movable P>
    struct scalar_payload final {
        using key_type = K;
        using value_type = P;
        using size_type = unsigned;
        using bundle_type = std::pair<key_type, value_type>;

        template<class K2 = K, class P2 = P>
        constexpr scalar_payload(K2&& key, P2&& value) noexcept(std::is_nothrow_move_constructible_v<K>)
            requires(std::constructible_from<std::pair<K, P>, std::pair<K2, P2>>)
             : _value(std::make_pair(std::forward<K2>(key), std::forward<P2>(value))) {
            LDBT_ZONE_A;
        }

        constexpr explicit scalar_payload(bundle_type&& bundle) noexcept(std::is_nothrow_constructible_v<bundle_type>)
             : _value(std::move(bundle)) {
            LDBT_ZONE_A;
        }

        constexpr explicit scalar_payload(const bundle_type& bundle) noexcept(std::is_nothrow_constructible_v<bundle_type>)
             : _value(bundle) {
            LDBT_ZONE_A;
        }

        constexpr scalar_payload() = default;

        constexpr scalar_payload(const scalar_payload& cp) = default;
        constexpr scalar_payload(scalar_payload&& mv) noexcept = default;

        constexpr ~scalar_payload() noexcept = default;

        constexpr scalar_payload&
        operator=(const scalar_payload& cp) = default;
        constexpr scalar_payload&
        operator=(scalar_payload&& mv) noexcept = default;

        [[nodiscard]] std::weak_ordering
        operator<=>(const auto& other) const noexcept(noexcept(kv_key() <=> other)) {
            LDBT_ZONE_A;
            if (empty()) return std::weak_ordering::equivalent;
            return kv_key() <=> other;
        }

        [[nodiscard]] constexpr size_type
        capacity() const noexcept { return 1U; }

        [[nodiscard]] constexpr size_type
        size() const noexcept {
            LDBT_ZONE_A;
            return _value.has_value() ? 1U : 0U;
        }

        [[nodiscard]] constexpr bool
        full() const noexcept {
            LDBT_ZONE_A;
            return _value.has_value();
        }

        [[nodiscard]] constexpr bool
        empty() const noexcept {
            LDBT_ZONE_A;
            return !_value.has_value();
        }

        bool
        try_merge(scalar_payload& other) {
            LDBT_ZONE_A;
            if (capacity() - size() < other.size()) return false;
            if (other.full()) {
                _value = other._value;
                other._value = std::nullopt;
            }
            return true;
        }

        void
        merge_until_full(scalar_payload& other) {
            LDBT_ZONE_A;
            std::ignore = try_merge(other);
        }

        template<index_lookup<value_type> Q>
        [[nodiscard]] constexpr std::optional<value_type>
        try_get(Q query) const noexcept(std::is_nothrow_constructible_v<std::optional<value_type>, value_type>) {
            LDBT_ZONE_A;
            if (empty()) return std::nullopt;
            if (kv_key() != query.key()) return std::nullopt;
            if (kv_value() != query) return std::nullopt;
            return {kv_value()};
        }

        [[nodiscard]] constexpr bool
        try_set(const key_type& key, const value_type& value) noexcept(noexcept(_value = std::make_pair(key, value))) {
            LDBT_ZONE_A;
            if (full()) {
                if (kv_key() == key) {
                    kv_value() = value;
                    return true;
                }
                return false;
            }
            _value = std::make_pair(key, value);
            return true;
        }

        [[nodiscard]] constexpr bool
        try_set(const bundle_type& bundle) noexcept(noexcept(try_set(bundle.first, bundle.second))) {
            LDBT_ZONE_A;
            return try_set(bundle.first, bundle.second);
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_lower(const K& key, const P& value) noexcept(noexcept(_value = std::make_pair(key, value))) {
            LDBT_ZONE_A;
            if (!empty() && kv_key() == key) {
                kv_value() = value;
                return std::nullopt;
            }
            auto res = std::move(_value);
            _value = std::make_pair(key, value);
            return res;
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_upper(const key_type& key, const value_type& value) noexcept(noexcept(force_set_lower(key, value))) {
            LDBT_ZONE_A;
            return force_set_lower(key, value);
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_lower(bundle_type&& bundle) noexcept(noexcept(force_set_lower(bundle.first, bundle.second))) {
            LDBT_ZONE_A;
            auto&& [key, value] = std::move(bundle);
            return force_set_lower(std::move(key), std::move(value));
        }

        [[nodiscard]] constexpr std::optional<bundle_type>
        force_set_upper(bundle_type&& bundle) noexcept(noexcept(force_set_upper(bundle.first, bundle.second))) {
            LDBT_ZONE_A;
            auto&& [key, value] = std::move(bundle);
            return force_set_upper(std::move(key), std::move(value));
        }

        template<index_lookup<value_type> Q>
        std::optional<value_type>
        remove(Q query) {
            LDBT_ZONE_A;
            if (!_value.has_value()) return std::nullopt;
            if (kv_key() == query.key()
                && kv_value() == query) return kv_value_destructive();
            return std::nullopt;
        }

        template<class Fn>
        void
        apply(Fn&& fn) {
            LDBT_ZONE_A;
            if (full()) std::invoke(std::forward<Fn>(fn), *_value);
        }

    private:
        [[nodiscard]] const key_type&
        kv_key() const noexcept {
            LDBT_ZONE_A;
            assert_that(!empty());
            return _value->first;
        }

        [[nodiscard]] const value_type&
        kv_value() const noexcept {
            LDBT_ZONE_A;
            assert_that(!empty());
            return _value->second;
        }

        [[nodiscard]] value_type
        kv_value_destructive() noexcept {
            LDBT_ZONE_A;
            assert_that(!empty());
            auto ret = _value->second;
            _value.reset();
            return ret;
        }

        [[nodiscard]] key_type&
        kv_key() noexcept {
            LDBT_ZONE_A;
            assert_that(!empty());
            return _value->first;
        }

        [[nodiscard]] value_type&
        kv_value() noexcept {
            LDBT_ZONE_A;
            assert_that(!empty());
            return _value->second;
        }

        friend constexpr std::ostream&
        operator<<(std::ostream& os, const scalar_payload& pl) {
            if (pl.empty()) return os << "(scalar: 1 0)";
            return os << "(scalar: 1 1 (" << pl._value->first << " " << pl._value->second << "))";
        }

        [[nodiscard]] friend constexpr bool
        operator==(const scalar_payload<K, P>& self, const K& other) noexcept(noexcept(self.kv_key() == other)) {
            LDBT_ZONE_A;
            if (self.empty()) return true;
            return self.kv_key() == other;
        }

        [[nodiscard]] friend constexpr bool
        operator==(const K& other, const scalar_payload<K, P>& self) noexcept(noexcept(other == self.kv_key())) {
            LDBT_ZONE_A;
            if (self.empty()) return true;
            return other == self.kv_key();
        }

        [[nodiscard]] friend constexpr bool
        operator!=(const scalar_payload<K, P>& self, const K& other) noexcept(noexcept(self.kv_key() != other)) {
            LDBT_ZONE_A;
            if (self.empty()) return false;
            return self.kv_key() != other;
        }

        [[nodiscard]] friend constexpr bool
        operator!=(const K& other, const scalar_payload<K, P>& self) noexcept(noexcept(other != self.kv_key())) {
            LDBT_ZONE_A;
            if (self.empty()) return false;
            return other != self.kv_key();
        }

        std::optional<std::pair<K, P>> _value{};
    };
    static_assert(payload<scalar_payload<int, int*>>);
}

#endif
