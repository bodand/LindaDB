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

#include <concepts>
#include <cstdint>
#include <span>
#include <typeinfo>
#include <variant>

#include <ldb/index/tree/tree.hxx>
#include <ldb/lv/linda_tuple.hxx>

namespace ldb {
    namespace meta {
        struct finder {
            explicit
            finder(size_t& idx) : idx(idx) { }
            std::size_t& idx;

            bool
            operator()(auto* ptr, std::size_t ptr_idx) const noexcept {
                if (ptr == nullptr) return false;
                idx = ptr_idx;
                return true;
            }
            bool
            operator()(bool same, std::size_t ptr_idx) const noexcept {
                if (!same) return false;
                idx = ptr_idx;
                return true;
            }
        };
    }

    template<class T>
    struct match_type {
        constexpr explicit
        match_type(T* ref) noexcept : _ref(ref) { }

        template<class... Args>
        [[nodiscard]] LDB_CONSTEXPR23 auto
        operator<=>(const std::variant<Args...>& value) const noexcept
            requires((std::same_as<T, Args> || ...))
        {
            LDB_PROF_SCOPE("MatchType_Variant");
            if (auto found = std::get_if<T>(&value);
                found) {
                *_ref = *found;
                return std::strong_ordering::equal;
            }

            auto valid_idx = [&value]<std::size_t... Is>(std::index_sequence<Is...>) {
                std::size_t idx{};
                (meta::finder(idx)(std::get_if<Args>(&value), Is) || ...);
                return idx;
            }(std::make_index_sequence<sizeof...(Args)>());
            auto t_idx = []<std::size_t... Is>(std::index_sequence<Is...>) {
                std::size_t idx{};
                (meta::finder(idx)(std::same_as<T, Args>, Is) || ...);
                return idx;
            }(std::make_index_sequence<sizeof...(Args)>());

            return valid_idx <=> t_idx;
        }

        constexpr static std::false_type
        indexable() { return {}; }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const match_type&) {
            return os << "Type(" << typeid(T).name() << ")";
        }

        T* _ref;
    };

    template<class T>
    struct match_value {
        constexpr
        match_value() noexcept = default;

        template<class U>
        explicit constexpr match_value(U&& field) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires(!std::same_as<U, match_value>)
             : _field(std::forward<U>(field)) { }

        explicit constexpr
        match_value(T& field)
             : _field(field) { }

        template<class... Args>
        [[nodiscard]] friend constexpr auto
        operator<=>(const std::variant<Args...>& value, const match_value& mv) noexcept(noexcept(std::declval<T>() == mv._field))
            requires((std::same_as<T, Args> || ...))
        {
            return std::visit([field = mv._field]<class V>(V&& val) {
                if constexpr (std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<V>>) {
                    LDB_PROF_SCOPE("MatchValue_VariantToValue_Value");
                    return field <=> std::forward<V>(val);
                }
                else {
                    LDB_PROF_SCOPE("MatchValue_VariantToValue_Type");
                    auto t_idx = []<std::size_t... Is>(std::index_sequence<Is...>) {
                        std::size_t idx{};
                        (meta::finder(idx)(std::same_as<T, Args>, Is) || ...);
                        return idx;
                    }(std::make_index_sequence<sizeof...(Args)>());
                    auto v_idx = []<std::size_t... Is>(std::index_sequence<Is...>) {
                        std::size_t idx{};
                        (meta::finder(idx)(std::same_as<V, Args>, Is) || ...);
                        return idx;
                    }(std::make_index_sequence<sizeof...(Args)>());
                    return v_idx <=> t_idx;
                }
            },
                              value);
        }

        constexpr static std::true_type
        indexable() { return {}; }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const match_value& val) {
            return os << val._field;
        }

        T _field;
    };

    template<class... Args>
    struct match_value<std::variant<Args...>> {
        constexpr
        match_value() noexcept = default;

        explicit constexpr
        match_value(const std::variant<Args...>& field) noexcept(std::is_nothrow_copy_constructible_v<std::variant<Args...>>)
             : _field(field) { }

        template<class... Args2>
        [[nodiscard]] friend LDB_CONSTEXPR23 auto
        operator<=>(const std::variant<Args2...>& value, const match_value& mv) noexcept(noexcept(mv._field <=> value)) {
            LDB_PROF_SCOPE("MatchValue_VariantToVariant");
            return value <=> mv._field;
        }

        constexpr static std::true_type
        indexable() { return {}; }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const match_value& val) {
            std::visit([&os](const auto& x) {
                os << x;
            },
                       val._field);
            return os;
        }

        std::variant<Args...> _field;
    };

    template<class T>
    match_value(T t) -> match_value<T>;

    template<class T>
    auto
    ref(T* ptr) { return match_type<T>{ptr}; }

    namespace meta {
        template<class T>
        struct make_matcher_impl {
            using type = match_value<T>;

            template<class U = T>
            constexpr auto
            operator()(U&& val) const noexcept {
                return match_value(std::forward<U>(val));
            }
        };

        template<class CharT, std::size_t N>
        struct make_matcher_impl<const CharT (&)[N]> {
            using type = match_value<std::basic_string<CharT>>;

            template<class U = const CharT*>
            constexpr auto
            operator()(U&& val) const noexcept {
                return match_value<std::basic_string<CharT>>(std::forward<U>(val));
            }
        };

        template<class P>
        struct make_matcher_impl<const match_type<P>&> {
            using type = match_type<P>;

            template<class M = match_type<P>>
            constexpr auto
            operator()(const M& matcher) const noexcept {
                return matcher;
            }
        };

        template<class P>
        struct make_matcher_impl<match_type<P>> {
            using type = match_type<P>;

            template<class M = match_type<P>>
            constexpr auto
            operator()(M matcher) const noexcept {
                return matcher;
            }
        };

        template<class T>
        constexpr const static auto make_matcher = make_matcher_impl<T>{};

        template<class T>
        using matcher_type = make_matcher_impl<T>::type;

        template<class TW>
        concept tuple_wrapper = requires(const TW& tw) {
            { *tw } -> std::same_as<lv::linda_tuple&>;
        };
    }

    template<class... Matcher>
    struct query_tuple {
        template<class... Args>
        explicit constexpr query_tuple(Args&&... args) noexcept((std::is_nothrow_constructible_v<Matcher, Args> && ...))
            requires((!std::same_as<Args, query_tuple> && ...))
             : _payload(meta::make_matcher<Args>(std::forward<Args>(args))...) { }

        template<class V, std::size_t C, class P, auto Ext = std::dynamic_extent>
        std::optional<std::optional<V>>
        try_read_indices(std::span<const index::tree::tree<lv::linda_value, V, C, P>, Ext> indices) const {
            LDB_PROF_SCOPE("QueryTuple_ReadIndex");
            std::optional<std::optional<V>> ret = std::nullopt; // top-level nullopt -> cannot use index

            [this, &ret, &indices]<std::size_t... Is>(std::index_sequence<Is...>) {
                auto aggregator = [this, &ret, &indices]<std::size_t Idx>() LDB_CONSTEXPR23 {
                    LDB_PROF_SCOPE("QueryTuple_ReadConcreteIndex");
                    if (!_indexable[Idx]) return true;
                    if (indices.size() < Idx) return false;
                    ret = std::optional(indices[Idx].search(index::tree::value_query(std::get<Idx>(_payload), *this)));
                    return !*ret;
                };
                (aggregator.template operator()<Is>() && ...);
            }(std::make_index_sequence<sizeof...(Matcher)>());

            return ret;
        }

        template<class V, std::size_t C, class P, auto Ext = std::dynamic_extent>
        std::pair<std::size_t, std::optional<std::optional<V>>>
        try_read_and_remove_indices(std::span<index::tree::tree<lv::linda_value, V, C, P>, Ext> indices) const {
            LDB_PROF_SCOPE("QueryTuple_ReadRemoveIndex");
            std::optional<std::optional<V>> ret = std::nullopt; // top-level nullopt -> cannot use index
            std::size_t index_idx{};

            [this, &index_idx, &ret, &indices]<std::size_t... Is>(std::index_sequence<Is...>) LDB_CONSTEXPR23 {
                auto aggregator = [this, &index_idx, &ret, &indices]<std::size_t Idx>() LDB_CONSTEXPR23 {
                    LDB_PROF_SCOPE("QueryTuple_ReadRemoveConcreteIndex");
                    if (!_indexable[Idx]) return true;
                    if (indices.size() < Idx) return false;
                    index_idx = Idx;
                    ret = std::optional(indices[Idx].remove(index::tree::value_query(std::get<Idx>(_payload), *this)));
                    return !*ret;
                };
                (aggregator.template operator()<Is>() && ...);
            }(std::make_index_sequence<sizeof...(Matcher)>());

            return {index_idx, ret};
        }

        std::string
        dump_string() const {
            std::stringstream ss;
            ss << (*this);
            return ss.str();
        }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const query_tuple& val) {
            [&os, &payload = val._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                (os << ... << std::get<Is>(payload));
            }(std::make_index_sequence<sizeof...(Matcher)>());
            return os;
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

        friend LDB_CONSTEXPR23 std::partial_ordering
        operator<=>(const lv::linda_tuple& lt, const query_tuple& query) {
            LDB_PROF_SCOPE("QueryTuple_Match");
            if (lt.size() != sizeof...(Matcher)) return lt.size() <=> sizeof...(Matcher);
            return [&lt, &payload = query._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                std::partial_ordering order = std::strong_ordering::equal;
                std::ignore = (matcher(order)(std::get<Is>(payload) <=> lt[Is]) && ...);
                return order;
            }(std::make_index_sequence<sizeof...(Matcher)>());
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend LDB_CONSTEXPR23 std::partial_ordering
        operator<=>(const TupleWrapper& tw, const query_tuple& query) {
            LDB_PROF_SCOPE("QueryTuple_MatchWrapper");
            if (tw->size() != sizeof...(Matcher)) return tw->size() <=> sizeof...(Matcher);
            return [&tw, &payload = query._payload]<std::size_t... Is>(std::index_sequence<Is...>) {
                std::partial_ordering order = std::strong_ordering::equal;
                std::ignore = (matcher(order)(std::get<Is>(payload) <=> (*tw)[Is]) && ...);
                return order;
            }(std::make_index_sequence<sizeof...(Matcher)>());
        }

        friend LDB_CONSTEXPR23 bool
        operator==(const lv::linda_tuple& lt, const query_tuple& query) {
            LDB_PROF_SCOPE("QueryTuple_MatchEq");
            return (lt <=> query) == 0;
        }

        template<meta::tuple_wrapper TupleWrapper>
        friend LDB_CONSTEXPR23 bool
        operator==(const TupleWrapper& tw, const query_tuple& query) {
            LDB_PROF_SCOPE("QueryTuple_MatchWrapperEq");
            return (*tw <=> query) == 0;
        }

        std::array<bool, sizeof...(Matcher)> _indexable{meta::matcher_type<Matcher>::indexable()...};
        std::tuple<meta::matcher_type<Matcher>...> _payload;
    };

    template<class... Matcher>
    query_tuple(Matcher&&... m) -> query_tuple<Matcher...>;
}

#endif
