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
 * src/LindaPq/pqlinda/src/linda_utils --
 *   
 */

#include <complex>

extern "C"
{
#include <postgres.h>
    // after postgres.h
#include <access/htup.h>
#include <access/htup_details.h>
#include <commands/async.h>
#include <utils/array.h>
#include <utils/rel.h>
}

#include "../../include/fam_datum.hxx"
#include "../../include/linda_funs.h"

int
lv_type(void* datum) {
    return static_cast<fam_datum*>(datum)->type();
}

const char*
lv_nicetype(void* datum) {
    switch (auto type = lv_type(datum);
            static_cast<fam_datum::datum_type>(type)) {
    case fam_datum::SINT16:
        return pstrdup("Int16");
    case fam_datum::UINT16:
        return pstrdup("UInt16");
    case fam_datum::SINT32:
        return pstrdup("Int32");
    case fam_datum::UINT32:
        return pstrdup("UInt32");
    case fam_datum::STRING:
        return pstrdup("String");
    case fam_datum::FNCALL:
        return pstrdup("RemoteFunctionCall");
    case fam_datum::SINT64:
        return pstrdup("Int64");
    case fam_datum::UINT64:
        return pstrdup("UInt64");
    case fam_datum::FLOT32:
        return pstrdup("Float32");
    case fam_datum::FLOT64:
        return pstrdup("Float64");
    case fam_datum::FNCTAG:
        return pstrdup("FunctionCallTag");
    case fam_datum::TYPERF:
        return pstrdup("TypeReference");
    default:
        pg_unreachable();
    }
}

namespace {
    char
    linda_type_int_to_char(int type) {
        switch (static_cast<fam_datum::datum_type>(type)) {
        case fam_datum::SINT16: return '0';
        case fam_datum::UINT16: return '1';
        case fam_datum::SINT32: return '2';
        case fam_datum::UINT32: return '3';
        case fam_datum::STRING: return '4';
        case fam_datum::FNCALL: return '5';
        case fam_datum::SINT64: return '6';
        case fam_datum::UINT64: return '7';
        case fam_datum::FLOT32: return '8';
        case fam_datum::FLOT64: return '9';
        case fam_datum::FNCTAG: return 'A';
        case fam_datum::TYPERF: return 'B';
        default:
            pg_unreachable();
        }
    }

    void
    send_notification(const char* payload) {
        Async_Notify("linda_event", payload);
    }
}

void
lv_notify_impl(void* tuple_raw, void* relation_raw) {
    auto* tuple = static_cast<HeapTuple>(tuple_raw);
    const auto* rel = static_cast<Relation>(relation_raw);
    auto* tupdesc = rel->rd_att;
    bool isnull = false;

    const int col_sz = tupdesc->natts;
    const int true_col_sz = col_sz - 1;
    const int array_col_idx = col_sz;

    // at least 1 attribute: id
    Assert(tupdesc->natts >= 1);
    // last attribute must be array
    Assert(tupdesc->attrs[array_col_idx]->attndim != 0);

    const auto id_datum = fastgetattr(tuple, 1, tupdesc, &isnull);
    const auto id_value = DatumGetInt64(id_datum);

    const auto id_str_len = static_cast<std::size_t>(std::floor(std::log10(id_value))) + 1U;
    const auto final_buf_size = static_cast<std::size_t>(col_sz) + id_str_len + 1U;
    std::string buf;
    buf.reserve(final_buf_size);
    buf.resize(id_str_len);

    // prepend id of inserted tuple for speedy search
    if (const auto [_ptr, ec] = std::to_chars(buf.data(),
                                              buf.data() + buf.size(),
                                              id_value);
        ec != std::errc{}) {
        const auto msg = std::make_error_code(ec).message();
        elog(ERROR, "error: cannot stringify id '%lu': %s", id_value, msg.c_str());
    }
    buf += ' ';

    // collect true column values' types
    // warning: non-standard for-loop goes from 1 with inclusive range
    for (int i = 2; i <= true_col_sz; ++i) {
        const Datum val = fastgetattr(tuple, i, tupdesc, &isnull);

        // the linda tuple ends on the first encountered SQL NULL
        if (isnull) break;

        auto maybe_detoasted = reinterpret_cast<Pointer>(PG_DETOAST_DATUM(val));
        buf += linda_type_int_to_char(lv_type(maybe_detoasted));
        if (val != reinterpret_cast<Datum>(maybe_detoasted)) pfree(maybe_detoasted);
    }

    // collect overflow array if exists
    bool no_overflow = true;
    const Datum overflow_array_raw = fastgetattr(tuple, array_col_idx, tupdesc, &no_overflow);
    ArrayType* overflow_array = nullptr;
    bool need_free_overflow_array = false;
    if (!no_overflow) {
        overflow_array = DatumGetArrayTypeP(overflow_array_raw);
        need_free_overflow_array = reinterpret_cast<Datum>(overflow_array) != overflow_array_raw;

        const auto array_sz = static_cast<std::size_t>(ArrayGetNItems(ARR_NDIM(overflow_array), ARR_DIMS(overflow_array)));
        buf.reserve(buf.size() + array_sz);

        const auto it = array_create_iterator(overflow_array, 0, nullptr);
        Datum elem; // NOLINT(*-init-variables)
        while (array_iterate(it, &elem, &isnull)) {
            // overflow arrays ignore nulls
            if (isnull) continue;

            auto maybe_detoasted = reinterpret_cast<Pointer>(PG_DETOAST_DATUM(elem));
            buf += linda_type_int_to_char(lv_type(maybe_detoasted));
            if (elem != reinterpret_cast<Datum>(maybe_detoasted)) pfree(maybe_detoasted);
        }
        array_free_iterator(it);
    }

    if (overflow_array && need_free_overflow_array) pfree(overflow_array);

    send_notification(buf.c_str());
}
