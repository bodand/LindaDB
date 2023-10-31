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
 * src/LindaDB/public/ldb/index/tree/index_query --
 *   A concept and related utilities that provide the functionality of searching
 *   index structures for LindaDB.
 */
#ifndef LINDADB_INDEX_QUERY_HXX
#define LINDADB_INDEX_QUERY_HXX

#include <concepts>
#include <tuple>

namespace ldb::index::tree {
    template<class Query, class Match>
    concept index_query = requires(Query query,
                                   Match value) {
        typename Query::key_type;

        { query.key() } -> std::same_as<const typename Query::key_type&>;
        { value <=> query };
        { value == query };
        { value != query };
    };

    template<class K>
    struct any_value_query {
        using key_type = K;

        explicit any_value_query(key_type key) noexcept(std::is_nothrow_move_constructible_v<key_type>)
             : _key(std::move(key)) { }

        [[nodiscard]] const key_type&
        key() const noexcept { return _key; }

    private:
        key_type _key;

        friend constexpr auto
        operator<=>(const auto& a, const any_value_query& b) noexcept {
            std::ignore = a;
            std::ignore = b;
            return std::strong_ordering::equal;
        }
        friend constexpr auto
        operator==(const auto& a, const any_value_query& b) noexcept {
            std::ignore = a;
            std::ignore = b;
            return true;
        }
    };
    static_assert(index_query<any_value_query<int>, int>);

    template<class K>
    any_value_query(const K&) -> any_value_query<K>;

    template<class K, class V>
    struct value_query {
        using key_type = K;
        using value_type = V;

        explicit value_query(key_type key,
                             value_type value) noexcept(std::is_nothrow_move_constructible_v<key_type>
                                                        && std::is_nothrow_move_constructible_v<value_type>)
             : _key(std::move(key)),
               _value(std::move(value)) { }

        [[nodiscard]] const key_type&
        key() const noexcept { return _key; }

    private:
        key_type _key;
        value_type _value;

        friend constexpr auto
        operator<=>(const auto& a, const value_query& b) noexcept {
            return a <=> b._value;
        }
        
        friend constexpr bool
        operator==(const auto& a, const value_query& b) noexcept {
            return a == b._value;
        }
    };
    static_assert(index_query<value_query<int, int>, int>);

    template<class K, class V>
    value_query(const K&, const V&) -> value_query<K, V>;
}

#endif
