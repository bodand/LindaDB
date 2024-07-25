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
 * Originally created: 2024-07-07.
 *
 * src/LindaPq/pqlinda/src/linda_funs --
 *   
 */

#include <cstring>

extern "C"
{
#include <postgres.h>
// after postgres.h
#include <fmgr.h>
}

#include "../include/fam_datum.hxx"
#include "../include/linda_funs.h"

namespace {
    template<class Int>
    Int
    pg_ntoh(Int inp) {
        static_assert(sizeof(Int) <= 8);

        if constexpr (sizeof(Int) == 1) return inp;
        if constexpr (sizeof(Int) == 2)
            return static_cast<Int>(pg_ntoh16(inp));
        else if constexpr (sizeof(Int) == 4)
            return static_cast<Int>(pg_ntoh32(inp));
        else if constexpr (sizeof(Int) == 8)
            return static_cast<Int>(pg_ntoh64(inp));
        pg_unreachable();
    }

    template<class Int>
    fam_datum*
    recv_integer(StringInfo bytes) {
        const auto* data = pq_getmsgbytes(bytes, sizeof(Int));

        Int ret;
        std::memcpy(reinterpret_cast<char * pg_restrict>(&ret),
                    data,
                    sizeof(Int));

        return fam_datum::create(pg_ntoh(ret));
    }
}

bytea*
internal_to_lv_bytes(void* datum) {
    return static_cast<fam_datum*>(datum)->to_bytes();
}

void*
lv_bytes_to_internal(StringInfo bytes) {
    const auto type = static_cast<fam_datum::datum_type>(pq_getmsgbyte(bytes));

    fam_datum* result;
    switch (type) {
    case fam_datum::SINT16:
        result = recv_integer<std::int16_t>(bytes);
        break;
    case fam_datum::UINT16:
        result = recv_integer<std::uint16_t>(bytes);
        break;
    case fam_datum::SINT32:
        result = recv_integer<std::int32_t>(bytes);
        break;
    case fam_datum::UINT32:
        result = recv_integer<std::uint32_t>(bytes);
        break;
    case fam_datum::STRING: {
        const char* str = pq_getmsgrawstring(bytes);
        result = fam_datum::create(str);
        if (str != bytes->data) pfree(const_cast<char*>(str));
        break;
    }
    case fam_datum::FNCALL:
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("invalid bytestring input for type %s: %s not implemented",
                        "linda_value",
                        "fncall")));
        break;
    case fam_datum::SINT64:
        result = recv_integer<std::int64_t>(bytes);
        break;
    case fam_datum::UINT64:
        result = recv_integer<std::uint64_t>(bytes);
        break;
    case fam_datum::FLOT32:
        result = fam_datum::create(pq_getmsgfloat4(bytes));
        break;
    case fam_datum::FLOT64:
        result = fam_datum::create(pq_getmsgfloat8(bytes));
        break;
    case fam_datum::FNCTAG:
        result = fam_datum::create_call_tag();
        break;
    case fam_datum::TYPERF:
        result = recv_integer<std::int8_t>(bytes);
        break;
    default:
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
                 errmsg("invalid input binary syntax for type %s: \"%d\"",
                        "linda_value",
                        type)));
    }

    pq_getmsgend(bytes);

    return result;
}
