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
 * Originally created: 2024-10-10.
 *
 * src/LindaDB/public/ldb/query/meta --
 *   Metaprogramming related helpers.
 */
#ifndef META_HXX
#define META_HXX

#include <ldb/common.hxx>

namespace ldb::meta {
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
    template<class T, template<class...> class L>
    struct index_of_type_i<T, L<>> {
    };

    template<class T, class TList>
    constexpr const static auto index_of_type = index_of_type_i<T, TList>::value;

    inline char
    to_hex(const int x) noexcept {
        constexpr static char hexa_buf[] = "ABCDEF";
        switch (x) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return x + '0';
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            return hexa_buf[x - 10];
        default:
            LDB_UNREACHABLE;
        }
    }
}

#endif
