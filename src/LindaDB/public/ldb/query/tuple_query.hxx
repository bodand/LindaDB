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
 * Originally created: 2024-01-15.
 *
 * src/LindaDB/public/ldb/query/tuple_query --
 *   
 */
#ifndef LINDADB_TUPLE_QUERY_HXX
#define LINDADB_TUPLE_QUERY_HXX

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>

#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <ldb/query/tuple_query_if.hxx>

namespace ldb {
    template<class IndexType>
    class tuple_query final {
        using internal_value_type = IndexType::value_type;

        struct query_concept {
            query_concept(const query_concept& cp) = delete;
            query_concept&
            operator=(const query_concept& cp) = delete;

            query_concept(query_concept&& mv) noexcept = delete;
            query_concept&
            operator=(query_concept&& mv) noexcept = delete;

            virtual ~query_concept() = default;

            [[nodiscard]] virtual field_match_type<internal_value_type>
            do_search_on_index(std::size_t field_index,
                               const IndexType& db_index) const = 0;

            [[nodiscard]] virtual field_match_type<internal_value_type>
            do_remove_on_index(std::size_t field_index,
                               IndexType& db_index) const = 0;

            [[nodiscard]] virtual std::partial_ordering
            do_compare(const lv::linda_tuple& tuple) const = 0;

            [[nodiscard]] virtual std::unique_ptr<query_concept>
            clone() const = 0;

        protected:
            query_concept() noexcept = default;
        };

        template<tuple_queryable Query>
        struct query_model final : query_concept {
            [[nodiscard]] field_match_type<internal_value_type>
            do_search_on_index(std::size_t field_index,
                               const IndexType& db_index) const override {
                return query_impl.search_via_field(field_index, db_index);
            }

            [[nodiscard]] field_match_type<internal_value_type>
            do_remove_on_index(std::size_t field_index,
                               IndexType& db_index) const override {
                return query_impl.remove_via_field(field_index, db_index);
            }

            [[nodiscard]] std::partial_ordering
            do_compare(const lv::linda_tuple& tuple) const override {
                return tuple <=> query_impl;
            }

            [[nodiscard]] std::unique_ptr<query_concept>
            clone() const override {
                return std::make_unique<query_model<Query>>(query_impl);
            }

            explicit query_model(Query query_impl)
                 : query_impl(std::move(query_impl)) { }

            Query query_impl;
        };

        std::unique_ptr<query_concept> _impl{};

        friend constexpr std::partial_ordering
        operator<=>(const lv::linda_tuple& lt, const tuple_query& query) {
            return query._impl->do_compare(lt);
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr std::partial_ordering
        operator<=>(const TupleWrapper& tw, const tuple_query& query) {
            return query._impl->do_compare(*tw);
        }

        friend constexpr bool
        operator==(const lv::linda_tuple& lt, const tuple_query& query) {
            return std::is_eq(lt <=> query);
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr bool
        operator==(const TupleWrapper& tw, const tuple_query& query) {
            return std::is_eq(*tw <=> query);
        }

    public:
        tuple_query() = default;

        template<class Query>
        explicit(false) tuple_query(Query&& query)
            requires(!std::same_as<Query, tuple_query>)
             : _impl(std::make_unique<query_model<Query>>(std::forward<Query>(query))) { }

        tuple_query(const tuple_query& cp)
             : _impl(cp._impl->clone()) { }
        tuple_query&
        operator=(const tuple_query& cp) {
            if (this != &cp) _impl = cp._impl->clone();
            return *this;
        }

        tuple_query(tuple_query&& mv) noexcept = default;
        tuple_query&
        operator=(tuple_query&& mv) noexcept = default;

        ~tuple_query() = default;

        [[nodiscard]] field_match_type<internal_value_type>
        search_on_index(std::size_t field_index,
                        const IndexType& db_index) const {
            assert(_impl);
            return _impl->do_search_on_index(field_index, db_index);
        }

        [[nodiscard]] field_match_type<internal_value_type>
        remove_on_index(std::size_t field_index,
                        IndexType& db_index) const {
            assert(_impl);
            return _impl->do_remove_on_index(field_index, db_index);
        }
    };
}

#endif
