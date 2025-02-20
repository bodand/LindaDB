/* LindaDB project
 *
 * Copyright (c) 2024 András Bodor <bodand@pm.me>
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
 * Originally created: 2024-01-17.
 *
 * src/LindaDB/public/ldb/query/tuple_query_if --
 *   Interface for a tuple query type.
 */
#ifndef LINDADB_TUPLE_QUERY_IF_HXX
#define LINDADB_TUPLE_QUERY_IF_HXX

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <variant>

#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>

namespace ldb {
    namespace meta {
        template<class TW>
        concept tuple_wrapper = requires(const TW& tw) {
            { *tw } -> std::same_as<lv::linda_tuple&>;
        };
    }

    struct field_incomparable { };
    struct field_not_found { };
    template<class T>
    struct field_found {
        T value;
    };

    template<class T>
    field_found(T) -> field_found<T>;

    template<class T>
    using field_match_type =
           std::variant<field_incomparable,
                        field_not_found,
                        field_found<T>>;

    template<class Query>
    concept tuple_queryable = requires(Query query,
                                       lv::linda_tuple tuple) {
        typename std::remove_cvref_t<Query>::value_type;

        { query.as_representing_tuple() } -> std::convertible_to<lv::linda_tuple>;
        { query.as_type_string() } -> std::same_as<std::string>;
        { query <=> tuple };
    };
}

#endif
