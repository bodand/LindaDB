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
 * src/LindaDB/public/ldb/query/match_type --
 *   A matcher field for matching any value with the given type.
 */
#ifndef LINDADB_MATCH_TYPE_HXX
#define LINDADB_MATCH_TYPE_HXX

#include <compare>
#include <concepts>
#include <cstddef>
#include <ostream>
#include <type_traits>
#include <utility>
#include <variant>

#include <ldb/lv/ref_type.hxx>
#include <ldb/query/meta_finder.hxx>

namespace ldb {
    namespace helper {
        template<class, class>
        struct index_of_type_i;

        template<class T, class Head, class... Tail, template<class...> class L>
        struct index_of_type_i<T, L<Head, Tail...>> {
            constexpr const static auto value = 1 + index_of_type_i<T, L<Tail...>>::value;
        };
        template<class T, class... Tail, template<class...> class L>
        struct index_of_type_i<T, L<T, Tail...>> {
            constexpr const static auto value = 0;
        };

        template<class T, class TList>
        constexpr const static auto index_of_type = index_of_type_i<T, TList>::value;
    }

    template<class T>
    struct match_type {
        constexpr explicit match_type(T* ref) noexcept : _ref(ref) { }

        template<class... Args>
        [[nodiscard]] constexpr auto
        operator<=>(const std::variant<Args...>& value) const noexcept
            requires((std::same_as<T, Args> || ...))
        {
            if (auto found = std::get_if<T>(&value);
                found) {
                if (_ref) *_ref = *found;
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

        template<class ValueList>
        [[nodiscard]] lv::ref_type
        get_value() const noexcept {
            return lv::ref_type(helper::index_of_type<T, ValueList>);
        }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const match_type&) {
            return os << "(::" << typeid(T).name() << ")";
        }

        T* _ref;
    };

    template<class T>
    constexpr const static auto type_checker = match_type<T>{nullptr};

    template<class T>
    auto
    ref(T* ptr) { return match_type<T>{ptr}; }
}

#endif
