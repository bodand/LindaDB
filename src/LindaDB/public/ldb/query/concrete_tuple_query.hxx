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
 * src/LindaDB/public/ldb/query/manual_fields_query --
 *   
 */
#ifndef LINDADB_CONCRETE_TUPLE_QUERY_HXX
#define LINDADB_CONCRETE_TUPLE_QUERY_HXX

#include <cassert>
#include <compare>
#include <cstddef>

#include <ldb/index/tree/index_query.hxx> // NOLINT(*-include-cleaner) actually used
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/tuple_query_if.hxx>

namespace ldb {
    template<class IndexType>
    struct concrete_tuple_query {
        using value_type = IndexType::value_type;

        concrete_tuple_query(const concrete_tuple_query& cp) = default;
        concrete_tuple_query&
        operator=(const concrete_tuple_query& cp) = default;

        concrete_tuple_query(concrete_tuple_query&& mv) noexcept = default;
        concrete_tuple_query&
        operator=(concrete_tuple_query&& mv) noexcept = default;

        ~concrete_tuple_query() noexcept = default;

        explicit concrete_tuple_query(const lv::linda_tuple& tuple)
             : _tuple(tuple) { }

        [[nodiscard]] field_match_type<value_type>
        search_via_field(std::size_t field_index,
                         const IndexType& db_index) const {
            assert(field_index < _tuple.size());
            if (const auto search_result = db_index.search(index::tree::value_lookup(_tuple[field_index], *this));
                search_result.has_value()) return field_found(*search_result);
            return field_not_found{};
        }

        [[nodiscard]] field_match_type<value_type>
        remove_via_field(std::size_t field_index,
                         IndexType& db_index) const {
            assert(field_index < _tuple.size());
            if (const auto search_result = db_index.remove(index::tree::value_lookup(_tuple[field_index], *this));
                search_result.has_value()) return field_found(*search_result);
            return field_not_found{};
        }

    private:
        lv::linda_tuple _tuple;

        friend constexpr bool
        operator==(const concrete_tuple_query& lhs,
                   const concrete_tuple_query& rhs) {
            return lhs._tuple == rhs._tuple;
        }

        friend constexpr bool
        operator!=(const concrete_tuple_query& lhs,
                   const concrete_tuple_query& rhs) {
            return !(lhs == rhs);
        }

        friend constexpr std::partial_ordering
        operator<=>(const lv::linda_tuple& lhs, const concrete_tuple_query& query) {
            const lv::linda_tuple& rhs = query._tuple;
            if (lhs.size() != rhs.size()) return lhs.size() <=> rhs.size();
            for (std::size_t i = 0; i < rhs.size(); ++i) {
                const auto cmp_result = lhs[i] <=> rhs[i];
                if (std::is_neq(cmp_result)) return cmp_result;
            }
            return std::strong_ordering::equal;
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr std::partial_ordering
        operator<=>(const TupleWrapper& tw, const concrete_tuple_query& query) {
            return *tw <=> query;
        }

        friend constexpr bool
        operator==(const lv::linda_tuple& lt, const concrete_tuple_query& query) {
            return std::is_eq(lt <=> query);
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr bool
        operator==(const TupleWrapper& tw, const concrete_tuple_query& query) {
            return std::is_eq(*tw <=> query);
        }
    };
}

#endif
