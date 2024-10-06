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

#include <charconv>
#include <cstring>
#include <string_view>

extern "C"
{
#include <postgres.h>
// after postgres.h
#include <fmgr.h>
}

#include "../../include/fam_datum.hxx"
#include "../../include/linda_funs.h"

namespace {
    [[noreturn]] void
    error_parsing_string(const char* val) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid input syntax for type %s: \"%s\"",
                        "linda_value",
                        val)));
    }

    template<class Int>
    fam_datum*
    parse_numeric(std::string_view data) {
        Int ret;
        const auto data_res = std::from_chars(data.data(), data.data() + data.size(), ret);
        if (data_res.ec != std::errc{} || *data_res.ptr != '\0')
            error_parsing_string(data.data());

        return fam_datum::create(ret);
    }
}

const char*
internal_to_lv_str(void* datum) {
    return static_cast<fam_datum*>(datum)->to_cstring();
}

void*
lv_str_to_internal(char* str_raw) {
    const auto str = std::string_view(str_raw);

    std::uint8_t data_type;
    const auto type_res = std::from_chars(str.data(), str.data() + str.size(), data_type, 16);
    if (type_res.ec != std::errc{} || *type_res.ptr != '@')
        error_parsing_string(str.data());

    const auto data_offset = static_cast<std::size_t>(type_res.ptr - str.data() + 1);

    fam_datum* result;
    switch (static_cast<fam_datum::datum_type>(data_type)) {
    case fam_datum::SINT16:
        result = parse_numeric<std::int16_t>(str.substr(data_offset));
        break;
    case fam_datum::UINT16:
        result = parse_numeric<std::uint16_t>(str.substr(data_offset));
        break;
    case fam_datum::SINT32:
        result = parse_numeric<std::int32_t>(str.substr(data_offset));
        break;
    case fam_datum::UINT32:
        result = parse_numeric<std::uint32_t>(str.substr(data_offset));
        break;
    case fam_datum::STRING:
        result = fam_datum::create(str.substr(data_offset));
        break;
    case fam_datum::FNCALL:
        // todo
        error_parsing_string("not implemented");
    case fam_datum::SINT64:
        result = parse_numeric<std::int64_t>(str.substr(data_offset));
        break;
    case fam_datum::UINT64:
        result = parse_numeric<std::uint64_t>(str.substr(data_offset));
        break;
    case fam_datum::FLOT32:
        result = parse_numeric<float>(str.substr(data_offset));
        break;
    case fam_datum::FLOT64:
        result = parse_numeric<double>(str.substr(data_offset));
        break;
    case fam_datum::FNCTAG:
        result = fam_datum::create_call_tag();
        break;
    case fam_datum::TYPERF:
        result = parse_numeric<std::int8_t>(str.substr(data_offset));
        break;
    default:
        error_parsing_string(str.data());
    }

    return result;
}
