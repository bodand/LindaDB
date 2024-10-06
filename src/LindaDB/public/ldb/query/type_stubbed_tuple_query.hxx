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
 * Originally created: 2024-04-04.
 *
 * src/LindaDB/public/ldb/query/type_stubbed_tuple_query --
 *   Type for handling queries made against tuples that contain ref_type
 *   values. These values must not be directly matched against.
 */
#ifndef LINDADB_TYPE_STUBBED_TUPLE_QUERY_HXX
#define LINDADB_TYPE_STUBBED_TUPLE_QUERY_HXX

#include <cassert>
#include <compare>
#include <cstddef>
#include <utility>

#include <ldb/index/tree/index_query.hxx> // NOLINT(*-include-cleaner) actually used
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/match_type.hxx>
#include <ldb/query/match_value.hxx>
#include <ldb/query/tuple_query_if.hxx>

namespace ldb {
    template<class IndexType>
    struct type_stubbed_tuple_query {
        using index_type = IndexType;
        using value_type = IndexType::value_type;

        type_stubbed_tuple_query(const type_stubbed_tuple_query& cp) = default;
        type_stubbed_tuple_query&
        operator=(const type_stubbed_tuple_query& cp) = default;

        type_stubbed_tuple_query(type_stubbed_tuple_query&& mv) noexcept = default;
        type_stubbed_tuple_query&
        operator=(type_stubbed_tuple_query&& mv) noexcept = default;

        ~
        type_stubbed_tuple_query() noexcept = default;

        explicit
        type_stubbed_tuple_query(const lv::linda_tuple& tuple)
             : _tuple(tuple) { }

        [[nodiscard]] field_match_type<value_type>
        search_via_field(std::size_t field_index,
                         const IndexType& db_index) const {
            LDBT_ZONE_A;
            assert_that(field_index < _tuple.size());
            return std::visit(search_query_visitor(
                                     _tuple[field_index],
                                     [&db_index]<class... Args>(Args&&... args) {
                                         return db_index.search(std::forward<Args>(args)...);
                                     },
                                     this),
                              _tuple[field_index]);
        }

        [[nodiscard]] field_match_type<value_type>
        remove_via_field(std::size_t field_index,
                         IndexType& db_index) const {
            LDBT_ZONE_A;
            assert_that(field_index < _tuple.size());
            return std::visit(search_query_visitor(
                                     _tuple[field_index],
                                     [&db_index]<class... Args>(Args&&... args) {
                                         return db_index.remove(std::forward<Args>(args)...);
                                     },
                                     this),
                              _tuple[field_index]);
        }

        [[nodiscard]] const lv::linda_tuple&
        as_representing_tuple() const noexcept {
            LDBT_ZONE_A;
            return _tuple;
        }

        [[nodiscard]] std::string
        as_type_string() const {
            LDBT_ZONE_A;
            return std::accumulate(_tuple.begin(), _tuple.end(), std::string(), [](std::string acc, const lv::linda_value& lv) {
                const auto last_size = acc.size();
                acc.resize(last_size + 1);
                std::to_chars(acc.data() + last_size, acc.data() + last_size + 1, lv.index(), 16);
                return acc;
            });
        }

    private:
        lv::linda_tuple _tuple;

        template<class Fn>
        struct search_query_visitor {
            search_query_visitor(const lv::linda_value& lhs,
                                 Fn opcode,
                                 const type_stubbed_tuple_query* self)
                 : _lhs(lhs),
                   _opcode(opcode),
                   _self(self) { }

            template<class T>
            field_match_type<value_type>
            operator()(const T&) const {
                if (const auto result = _opcode(index::tree::value_lookup(_lhs, *_self));
                    result) return field_found(*result);
                return field_not_found{};
            }

            field_match_type<value_type>
            operator()(ldb::lv::ref_type) const {
                return field_incomparable{};
            }

        private:
            const lv::linda_value& _lhs;
            Fn _opcode;
            const type_stubbed_tuple_query* _self;
        };

        template<class Fn>
        search_query_visitor(const lv::linda_value& lhs,
                             Fn opcode,
                             const type_stubbed_tuple_query* self) -> search_query_visitor<Fn>;

        friend constexpr bool
        operator==(const type_stubbed_tuple_query& lhs,
                   const type_stubbed_tuple_query& rhs) {
            return lhs._tuple == rhs._tuple;
        }

        friend constexpr bool
        operator!=(const type_stubbed_tuple_query& lhs,
                   const type_stubbed_tuple_query& rhs) = default;

        friend constexpr std::partial_ordering
        operator<=>(const lv::linda_tuple& lhs, const type_stubbed_tuple_query& query) {
            const lv::linda_tuple& rhs = query._tuple;
            if (lhs.size() != rhs.size()) return lhs.size() <=> rhs.size();
            for (std::size_t i = 0; i < rhs.size(); ++i) {
                const auto cmp_result = std::visit(type_aware_lv_comparer(rhs[i], lhs[i]), rhs[i]);
                if (std::is_neq(cmp_result)) return cmp_result;
            }
            return std::strong_ordering::equal;
        }

        /// Special comparer that treads ref_type specially
        struct type_aware_lv_comparer {
            const lv::linda_value& rhs;
            const lv::linda_value& lhs;

            constexpr std::partial_ordering
            operator()(const auto&) const {
                return lhs <=> rhs;
            }

            constexpr std::partial_ordering
            operator()(lv::ref_type ref) const {
                return lhs <=> ref;
            }
        };

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr std::partial_ordering
        operator<=>(const TupleWrapper& tw, const type_stubbed_tuple_query& query) {
            return *tw <=> query;
        }

        friend constexpr bool
        operator==(const lv::linda_tuple& lt, const type_stubbed_tuple_query& query) {
            return std::is_eq(lt <=> query);
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr bool
        operator==(const TupleWrapper& tw, const type_stubbed_tuple_query& query) {
            return std::is_eq(*tw <=> query);
        }

        friend std::ostream&
        operator<<(std::ostream& os, const type_stubbed_tuple_query& query) {
            return os << "QUERY(" << query._tuple << ")";
        }
    };
}

#endif
