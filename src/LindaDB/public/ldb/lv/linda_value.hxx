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
 * Originally created: 2023-10-18.
 *
 * src/LindaDB/public/ldb/lv/linda_value --
 */

#ifndef LINDADB_LINDA_VALUE_HXX
#define LINDADB_LINDA_VALUE_HXX

#include <concepts>
#include <typeinfo>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include <ldb/lv/fn_call_holder.hxx>
#include <ldb/lv/fn_call_tag.hxx>

namespace ldb::lv {
    using linda_value = std::variant<
           std::int16_t,
           std::uint16_t,
           std::int32_t,
           std::uint32_t,
           std::int64_t,
           std::uint64_t,
           std::string,
           float,
           double,
           fn_call_holder,
           fn_call_tag>;

    template<class T>
    inline linda_value
    make_linda_value(T&& val) { return linda_value(std::forward<T>(val)); }

    inline linda_value
    make_linda_value(std::string_view val) { return linda_value(std::string(val)); }

    inline linda_value
    make_linda_value(const std::string& val) { return linda_value(val); }

    namespace helper {
        struct printer {
            explicit printer(std::ostream& os) : _os(os) { }

            template<class T>
            void
            operator()(T val)
                requires(std::is_trivially_copyable_v<T>)
            { _os << "(lv: " << val << "::" << typeid(T).name() << ")"; }
            template<class T>
            void
            operator()(const T& val)
                requires(!std::is_trivially_copyable_v<T>)
            { _os << "(lv: " << val << "::" << typeid(T).name() << ")"; }

            void
            operator()(const std::string& val)
            { _os << "(lv: " << val << "@" << val.size() << "::string)"; }

        private:
            std::ostream& _os;
        };

        template<class T, class L>
        struct is_member_of : std::false_type { };

        template<class T, template<class...> class L, class... Args>
        struct is_member_of<T, L<Args...>> : std::bool_constant<(std::same_as<T, Args> || ...)> { };
    }

    template<class T>
    struct is_linda_value : std::bool_constant<helper::is_member_of<T, linda_value>::value> { };

    template<std::size_t N>
    struct is_linda_value<const char[N]> : std::true_type { };
    template<std::size_t N>
    struct is_linda_value<char[N]> : std::true_type { };

    template<class T>
    constexpr const static auto is_linda_value_v = is_linda_value<T>::value;

    inline namespace io {
        inline std::ostream&
        operator<<(std::ostream& os, const linda_value& lv) {
            std::visit(helper::printer(os), lv);
            return os;
        }
    }
}

#endif //LINDADB_LINDA_VALUE_HXX
