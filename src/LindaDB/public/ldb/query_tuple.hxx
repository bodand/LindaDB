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
 * Originally created: 2023-10-22.
 *
 * src/LindaDB/public/ldb/query_tuple --
 *   
 */
#ifndef LINDADB_QUERY_TUPLE_HXX
#define LINDADB_QUERY_TUPLE_HXX

#include <array>
#include <compare>
#include <concepts>
#include <cstdlib>
#include <optional>
#include <span>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <variant>

#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <ldb/query/make_matcher.hxx>
#include <ldb/query/match_type.hxx>
#include <ldb/query/match_value.hxx>
#include <ldb/query/tuple_query_if.hxx>

namespace ldb {
    template<class... Matcher>
    struct query_tuple {
        template<class... Args>
        explicit constexpr query_tuple(Args&&... args) noexcept((std::is_nothrow_constructible_v<Matcher, Args> && ...))
            requires((!std::same_as<Args, query_tuple> && ...))
             : _payload(meta::make_matcher<Args>(std::forward<Args>(args))...) { }

        template<class V, std::size_t C, class P, auto Ext = std::dynamic_extent>
        std::optional<std::optional<V>>
        try_read_indices(std::span<const index::tree::avl2_tree<lv::linda_value, V, C, P>, Ext> indices) const {
            std::optional<std::optional<V>> ret = std::nullopt; // top-level nullopt -> cannot use index

            [this, &ret, &indices]<std::size_t... Is>(std::index_sequence<Is...>) {
                auto aggregator = [this, &ret, &indices]<std::size_t Idx>() constexpr {
                    if (!_indexable[Idx]) return true;
                    if (indices.size() < Idx) return false;
                    ret = std::optional(indices[Idx].search(index::tree::value_lookup(std::get<Idx>(_payload), *this)));
                    return !*ret;
                };
                (aggregator.template operator()<Is>() && ...);
            }(std::make_index_sequence<sizeof...(Matcher)>());

            return ret;
        }

        template<class V, std::size_t C, class P, auto Ext = std::dynamic_extent>
        std::pair<std::size_t, std::optional<std::optional<V>>>
        try_read_and_remove_indices(std::span<index::tree::avl2_tree<lv::linda_value, V, C, P>, Ext> indices) const {
            std::optional<std::optional<V>> ret = std::nullopt; // top-level nullopt -> cannot use index
            std::size_t index_idx{};

            [this, &index_idx, &ret, &indices]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
                auto aggregator = [this, &index_idx, &ret, &indices]<std::size_t Idx>() constexpr {
                    if (!_indexable[Idx]) return true;
                    if (indices.size() < Idx) return false;
                    index_idx = Idx;
                    ret = std::optional(indices[Idx].remove(index::tree::value_lookup(std::get<Idx>(_payload), *this)));
                    return !*ret;
                };
                std::ignore = (aggregator.template operator()<Is>() && ...);
            }(std::make_index_sequence<sizeof...(Matcher)>());

            return {index_idx, ret};
        }

    private:
        template<class IndexType, class... Args>
        friend struct manual_fields_query;

        friend std::ostream&
        operator<<(std::ostream& os, const query_tuple& val) {
            [&os, &payload = val._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                (os << ... << std::get<Is>(payload));
            }(std::make_index_sequence<sizeof...(Matcher)>());
            return os;
        }

        struct matcher {
            explicit matcher(std::partial_ordering& ordering) : ordering(ordering) { }
            std::partial_ordering& ordering;

            bool
            operator()(std::partial_ordering order) const noexcept {
                if (std::is_eq(order)) return true;
                ordering = order;
                return false;
            }
        };

        friend constexpr std::partial_ordering
        operator<=>(const lv::linda_tuple& lt, const query_tuple& query) {
            if (lt.size() != sizeof...(Matcher)) return lt.size() <=> sizeof...(Matcher);
            return [&lt, &payload = query._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                std::partial_ordering order = std::strong_ordering::equal;
                std::ignore = (matcher(order)(std::get<Is>(payload) <=> lt[Is]) && ...);
                return order;
            }(std::make_index_sequence<sizeof...(Matcher)>());
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr std::partial_ordering
        operator<=>(const TupleWrapper& tw, const query_tuple& query) {
            if (tw->size() != sizeof...(Matcher)) return tw->size() <=> sizeof...(Matcher);
            return [&tw, &payload = query._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                std::partial_ordering order = std::strong_ordering::equal;
                std::ignore = (matcher(order)(std::get<Is>(payload) <=> (*tw)[Is]) && ...);
                return order;
            }(std::make_index_sequence<sizeof...(Matcher)>());
        }

        friend constexpr bool
        operator==(const lv::linda_tuple& lt, const query_tuple& query) {
            return (lt <=> query) == 0;
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr bool
        operator==(const TupleWrapper& tw, const query_tuple& query) {
            return (*tw <=> query) == 0;
        }

        std::array<bool, sizeof...(Matcher)> _indexable{meta::matcher_type<Matcher>::indexable()...};
        std::tuple<meta::matcher_type<Matcher>...> _payload;
    };

    template<class... Matcher>
    query_tuple(Matcher&&... m) -> query_tuple<Matcher...>;
}

#endif
