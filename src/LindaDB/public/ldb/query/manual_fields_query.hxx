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
#ifndef LINDADB_MANUAL_FIELDS_QUERY_HXX
#define LINDADB_MANUAL_FIELDS_QUERY_HXX

#include <compare>
#include <concepts>
#include <cstddef>
#include <fstream>
#include <tuple>
#include <utility>

#include <ldb/index/tree/index_query.hxx> // NOLINT(*-include-cleaner) actually used
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/make_matcher.hxx>
#include <ldb/query/tuple_query_if.hxx>

namespace ldb {
    template<class IndexType, class... Matchers>
    struct manual_fields_query final {
        using index_type = IndexType;
        using value_type = IndexType::value_type;

        manual_fields_query(const manual_fields_query& cp) = default;
        manual_fields_query&
        operator=(const manual_fields_query& cp) = default;

        manual_fields_query(manual_fields_query&& mv) noexcept = default;
        manual_fields_query&
        operator=(manual_fields_query&& mv) noexcept = default;

        ~
        manual_fields_query() noexcept = default;

        template<class... Args>
        explicit constexpr manual_fields_query(Args&&... args) noexcept((
               std::is_nothrow_constructible_v<Matchers, Args> && ...))
            requires((!std::same_as<Args, manual_fields_query> && ...))
             : _payload(meta::make_matcher<Args>(std::forward<Args>(args))...) { }

        [[nodiscard]] lv::linda_tuple
        as_representing_tuple() const noexcept {
            LDBT_ZONE_A;
            return [this]<std::size_t... MatcherIndex>(
                          std::index_sequence<MatcherIndex...>) {
                return lv::linda_tuple(
                       lv::make_linda_value(
                              std::get<MatcherIndex>(_payload).template get_value<lv::linda_value>())...);
            }(std::make_index_sequence<sizeof...(Matchers)>());
        }

        [[nodiscard]] std::string
        as_type_string() const noexcept {
            LDBT_ZONE_A;
            std::string ret(sizeof...(Matchers), '0');
            [this, &ret]<std::size_t... MatcherIndex>(
                          std::index_sequence<MatcherIndex...>) {
                (((void) (ret[MatcherIndex] = std::get<MatcherIndex>(_payload).template get_type_char<lv::linda_value>())), ...);
            }(std::make_index_sequence<sizeof...(Matchers)>());
            return ret;
        }

    private:
        constexpr const static auto CONTINUE_LOOP = true;
        constexpr const static auto TERMINATE_LOOP = false;

        template<class Fn>
        [[nodiscard]] field_match_type<value_type>
        iterate_matchers_via(Fn&& fn) const {
            field_match_type<value_type> result = field_incomparable{};

            [&result, callback = std::forward<Fn>(fn), this]<std::size_t... MatcherIndex>(
                   std::index_sequence<MatcherIndex...>) {
                std::ignore = (callback(result,
                                        MatcherIndex,
                                        std::get<MatcherIndex>(_payload))
                               && ...);
            }(std::make_index_sequence<sizeof...(Matchers)>());
            return result;
        }

        template<class MatcherImplType>
        [[nodiscard]] field_match_type<value_type>
        perform_search(const MatcherImplType& matcher_impl,
                       const IndexType& db_index) const {
            if (!matcher_impl.indexable()) return field_incomparable{};
            if (const auto search_result = db_index.search(index::tree::value_lookup(matcher_impl, *this));
                search_result.has_value()) return field_found(*search_result);
            return field_not_found{};
        }

        template<class MatcherImplType>
        [[nodiscard]] field_match_type<value_type>
        perform_remove(const MatcherImplType& matcher_impl,
                       IndexType& db_index) const {
            if (!matcher_impl.indexable()) return field_incomparable{};
            if (const auto search_result = db_index.remove(index::tree::value_lookup(matcher_impl, *this));
                search_result.has_value()) return field_found(*search_result);
            return field_not_found{};
        }

        struct matcher {
            explicit
            matcher(std::partial_ordering& ordering) : ordering(ordering) { }
            std::partial_ordering& ordering;

            bool
            operator()(std::partial_ordering order) const noexcept {
                if (std::is_eq(order)) return true;
                ordering = order;
                return false;
            }
        };

        friend constexpr std::partial_ordering
        operator<=>(const lv::linda_tuple& lt, const manual_fields_query& query) {
            if (lt.size() != sizeof...(Matchers)) return lt.size() <=> sizeof...(Matchers);
            return [&lt, &payload = query._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                std::partial_ordering order = std::strong_ordering::equal;
                std::ignore = (matcher(order)(lt[Is] <=> std::get<Is>(payload)) && ...);
                return order;
            }(std::make_index_sequence<sizeof...(Matchers)>());
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr std::partial_ordering
        operator<=>(const TupleWrapper& tw, const manual_fields_query& query) {
            return *tw <=> query;
        }

        friend constexpr bool
        operator==(const lv::linda_tuple& lt, const manual_fields_query& query) {
            return (lt <=> query) == 0;
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend constexpr bool
        operator==(const TupleWrapper& tw, const manual_fields_query& query) {
            return (*tw <=> query) == 0;
        }

        friend std::ostream&
        operator<<(std::ostream& os, const manual_fields_query& query) {
            [&os, &payload = query._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                os << "QUERY(";
                ((os << "," << std::get<Is>(payload)), ...);
                os << ")";
            }(std::make_index_sequence<sizeof...(Matchers)>());
            return os;
        }

        std::tuple<meta::matcher_type<Matchers>...> _payload;
    };
}

#endif
