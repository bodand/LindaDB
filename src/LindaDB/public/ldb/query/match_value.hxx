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
 * src/LindaDB/public/ldb/query/match_value --
 *   
 */
#ifndef LINDADB_MATCH_VALUE_HXX
#define LINDADB_MATCH_VALUE_HXX

#include <concepts>
#include <ostream>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>

namespace ldb {
    template<class T>
    struct match_value {
        constexpr match_value() noexcept = default;

        template<class U>
        explicit constexpr match_value(U&& field) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires(!std::same_as<U, match_value>)
             : _field(std::forward<U>(field)) { }

        explicit constexpr match_value(T& field)
             : _field(field) { }

        template<class... Args>
        [[nodiscard]] friend constexpr auto
        operator<=>(const std::variant<Args...>& value, const match_value& mv) noexcept(noexcept(std::declval<T>() == mv._field))
            requires((std::same_as<T, Args> || ...))
        {
            return std::visit([field = mv._field]<class V>(V&& val) {
                if constexpr (std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<V>>) {
                    return std::forward<V>(val) <=> field;
                }
                else {
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
                    return t_idx <=> v_idx;
                }
            },
                              value);
        }

        constexpr static std::true_type
        indexable() { return {}; }

        template<class ValueList>
        [[nodiscard]] std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>
        get_value() const noexcept {
            return _field;
        }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const match_value& val) {
            if constexpr (std::same_as<T, std::string>) {
                return os << "(value: " << val._field << "@" << val._field.size() << "::string)";
            }
            else {
                return os << "(value: " << val._field << "::" << typeid(T).name() << ")";
            }
        }

        T _field;
    };

    template<class... Args>
    struct match_value<std::variant<Args...>> {
        constexpr match_value() noexcept = default;

        explicit constexpr match_value(const std::variant<Args...>& field) noexcept(std::is_nothrow_copy_constructible_v<std::variant<Args...>>)
             : _field(field) { }

        template<class... Args2>
        [[nodiscard]] friend constexpr auto
        operator<=>(const std::variant<Args2...>& value, const match_value& mv) noexcept(noexcept(mv._field <=> value)) {
            return value <=> mv._field;
        }

        constexpr static std::true_type
        indexable() { return {}; }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const match_value& val) {
            std::visit([&os](const auto& x) { os << x; },
                       val._field);
            return os;
        }

        std::variant<Args...> _field;
    };

    template<class T>
    match_value(T t) -> match_value<T>;
}

#endif
