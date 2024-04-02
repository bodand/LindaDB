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
 * Originally created: 2024-04-04.
 *
 * src/LindaDB/public/ldb/lv/ref_type --
 *   
 */
#ifndef LINDADB_REF_TYPE_HXX
#define LINDADB_REF_TYPE_HXX

#include <compare>
#include <cstdint>
#include <ostream>

namespace ldb::lv {
    struct ref_type {
        using value_type = std::int8_t;

        constexpr explicit ref_type(std::size_t type_idx)
             : ref_type(static_cast<value_type>(type_idx)) { }
        constexpr explicit ref_type(int type_idx)
             : ref_type(static_cast<value_type>(type_idx)) { }
        constexpr explicit ref_type(value_type type_idx)
             : _type_idx(type_idx) { }

        [[nodiscard]] value_type
        type_idx() const noexcept {
            return _type_idx;
        }

    private:
        value_type _type_idx;

        friend constexpr auto
        operator<=>(ref_type a, ref_type b) = default;

        friend constexpr auto
        operator<=>(std::size_t idx, ref_type b) {
            return idx <=> static_cast<std::size_t>(b._type_idx);
        }

        friend constexpr bool
        operator==(ref_type a, ref_type b) = default;

        friend std::ostream&
        operator<<(std::ostream& os, ref_type t) {
            return os << "(type: " << static_cast<int>(t._type_idx) << ")";
        }
    };
}

namespace std {
    template<>
    struct hash<ldb::lv::ref_type> {
        std::size_t
        operator()(const ldb::lv::ref_type& ref) const noexcept {
            return hash<int>{}(ref.type_idx());
        }
    };
}

#endif
