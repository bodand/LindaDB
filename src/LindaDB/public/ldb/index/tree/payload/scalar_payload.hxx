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
#include <concepts>

#include <ldb/index/tree/payload.hxx>
#include <ldb/profiler.hxx>

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
             : _value(std::make_pair(std::forward<K2>(key), std::forward<P2>(value))) { }

        constexpr explicit scalar_payload(bundle_type&& bundle) noexcept(std::is_nothrow_constructible_v<bundle_type>)
             : _value(std::move(bundle)) { }

        constexpr scalar_payload() = default;

        constexpr scalar_payload(const scalar_payload& cp) = default;
        constexpr scalar_payload(scalar_payload&& mv) noexcept = default;

        constexpr ~scalar_payload() noexcept = default;

        constexpr scalar_payload&
        operator=(const scalar_payload& cp) = default;
        constexpr scalar_payload&
        operator=(scalar_payload&& mv) noexcept = default;

        [[nodiscard]] std::weak_ordering
        operator<=>(const K& other) const noexcept(noexcept(kv_key() <=> other)) {
            LDB_PROF_SCOPE("ScalarPayload_Ordering");
            if (empty()) return std::weak_ordering::equivalent;
            return kv_key() <=> other;
        }

        [[nodiscard]] constexpr size_type
        capacity() const noexcept { return 1U; }

        [[nodiscard]] constexpr size_type
        size() const noexcept { return _value.has_value() ? 1U : 0U; }

        [[nodiscard]] constexpr bool
        full() const noexcept { return _value.has_value(); }

        [[nodiscard]] constexpr bool
        empty() const noexcept { return !_value.has_value(); }

        [[nodiscard]] constexpr bool
        have_priority() const noexcept { return false; }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<value_type>
        try_get(const key_type& key) const noexcept(std::is_nothrow_constructible_v<std::optional<value_type>, value_type>) {
            LDB_PROF_SCOPE_C("ScalarPayload_Search", ldb::prof::color_search);
            if (empty()) return std::nullopt;
            if (kv_key() != key) return std::nullopt;
            return {kv_value()};
        }

        [[nodiscard]] LDB_CONSTEXPR23 bool
        try_set(const key_type& key, const value_type& value) noexcept(noexcept(_value = std::make_pair(key, value))) {
            LDB_PROF_SCOPE_C("ScalarPayload_Insert", ldb::prof::color_insert);
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

        [[nodiscard]] LDB_CONSTEXPR23 bool
        try_set(const bundle_type& bundle) noexcept(noexcept(try_set(bundle.first, bundle.second))) {
            return try_set(bundle.first, bundle.second);
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_lower(const K& key, const P& value) noexcept(noexcept(_value = std::make_pair(key, value))) {
            LDB_PROF_SCOPE_C("ScalarPayload_InsertAndSquish", ldb::prof::color_insert);
            if (!empty() && kv_key() == key) {
                kv_value() = value;
                return std::nullopt;
            }
            auto res = std::move(_value);
            _value = std::make_pair(key, value);
            return res;
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_upper(const key_type& key, const value_type& value) noexcept(noexcept(force_set_lower(key, value))) {
            return force_set_lower(key, value);
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_lower(bundle_type&& bundle) noexcept(noexcept(force_set_lower(bundle.first, bundle.second))) {
            auto&& [key, value] = std::move(bundle);
            return force_set_lower(std::move(key), std::move(value));
        }

        [[nodiscard]] LDB_CONSTEXPR23 std::optional<bundle_type>
        force_set_upper(bundle_type&& bundle) noexcept(noexcept(force_set_upper(bundle.first, bundle.second))) {
            auto&& [key, value] = std::move(bundle);
            return force_set_upper(std::move(key), std::move(value));
        }

        std::optional<value_type>
        remove(const K& key) {
            if (kv_key() == key) return kv_value_destructive();
            return std::nullopt;
        }

    private:
        [[nodiscard]] const key_type&
        kv_key() const noexcept {
            assert(!empty());
            return _value->first;
        }

        [[nodiscard]] const value_type&
        kv_value() const noexcept {
            assert(!empty());
            return _value->second;
        }

        [[nodiscard]] value_type
        kv_value_destructive() noexcept {
            assert(!empty());
            auto ret = _value->second;
            _value.reset();
            return ret;
        }

        [[nodiscard]] key_type&
        kv_key() noexcept {
            assert(!empty());
            return _value->first;
        }

        [[nodiscard]] value_type&
        kv_value() noexcept {
            assert(!empty());
            return _value->second;
        }

        friend constexpr std::ostream&
        operator<<(std::ostream& os, const scalar_payload& pl) {
            if (pl.empty()) return os << "(1 0)";
            return os << "(1 1 (" << pl._value->first << " " << pl._value->second << "))";
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator==(const scalar_payload<K, P>& self, const K& other) noexcept(noexcept(self.kv_key() == other)) {
            LDB_PROF_SCOPE("ScalarPayload_LeftEq");
            if (self.empty()) return true;
            return self.kv_key() == other;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator==(const K& other, const scalar_payload<K, P>& self) noexcept(noexcept(other == self.kv_key())) {
            LDB_PROF_SCOPE("ScalarPayload_RightEq");
            if (self.empty()) return true;
            return other == self.kv_key();
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator!=(const scalar_payload<K, P>& self, const K& other) noexcept(noexcept(self.kv_key() != other)) {
            LDB_PROF_SCOPE("ScalarPayload_LeftNe");
            if (self.empty()) return false;
            return self.kv_key() != other;
        }

        [[nodiscard]] friend LDB_CONSTEXPR23 bool
        operator!=(const K& other, const scalar_payload<K, P>& self) noexcept(noexcept(other != self.kv_key())) {
            LDB_PROF_SCOPE("ScalarPayload_RightNe");
            if (self.empty()) return false;
            return other != self.kv_key();
        }

        std::optional<std::pair<K, P>> _value{};
    };
    static_assert(payload<scalar_payload<int, int*>>);
}

#endif
