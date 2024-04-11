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
 * Originally created: 2024-03-02.
 *
 * src/LindaDB/public/ldb/lv/fn_call_tag --
 *   
 */
#ifndef LINDADB_FN_CALL_TAG_HXX
#define LINDADB_FN_CALL_TAG_HXX

#include <compare>

#include <ldb/profile.hxx>

namespace ldb::lv {
    struct fn_call_tag {
    private:
        friend auto
        operator<=>(fn_call_tag, fn_call_tag) {
            LDBT_ZONE_A;
            return std::strong_ordering::equal;
        }

        template<class T>
        friend constexpr auto
        operator<=>(fn_call_tag, const T&) {
            LDBT_ZONE_A;
            return std::strong_ordering::less;
        }

        template<class T>
        friend auto
        operator<=>(const T&, fn_call_tag) {
            LDBT_ZONE_A;
            return std::strong_ordering::greater;
        }

        friend auto
        operator==(fn_call_tag, fn_call_tag) {
            LDBT_ZONE_A;
            return true;
        }

        template<class T>
        friend auto
        operator==(fn_call_tag, const T&) {
            LDBT_ZONE_A;
            return false;
        }

        template<class T>
        friend auto
        operator==(const T&, fn_call_tag) {
            LDBT_ZONE_A;
            return false;
        }
    };
}

namespace std {
    template<>
    struct hash<ldb::lv::fn_call_tag> {
        std::size_t
        operator()(const ldb::lv::fn_call_tag& /*ignored*/) const noexcept {
            LDBT_ZONE_A;
            return static_cast<std::size_t>(0xC0FFEE'0F'DEADBEEF);
        }
    };
}


#endif
