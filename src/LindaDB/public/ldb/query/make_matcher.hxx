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
 * src/LindaDB/public/ldb/query/make_matcher --
 *   
 */
#ifndef LINDADB_MAKE_MATCHER_HXX
#define LINDADB_MAKE_MATCHER_HXX

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include <ldb/query/match_type.hxx>
#include <ldb/query/match_value.hxx>

namespace ldb::meta {
    template<class T>
    struct make_matcher_impl {
        using type = match_value<T>;

        template<class U = T>
        constexpr auto
        operator()(U&& val) const noexcept {
            return match_value(std::forward<U>(val));
        }
    };

    template<class T>
    struct make_matcher_impl<const T&> {
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

    template<class CharT, std::size_t N>
    struct make_matcher_impl<CharT[N]> {
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

    template<class T>
    struct is_matcher_type : std::false_type {};
    template<class T>
    struct is_matcher_type<match_type<T>> : std::true_type {};
    template<class T>
    struct is_matcher_type<match_value<T>> : std::true_type {};

    template<class T>
    constexpr const static auto is_matcher_type_v = is_matcher_type<T>::value;
}

#endif
