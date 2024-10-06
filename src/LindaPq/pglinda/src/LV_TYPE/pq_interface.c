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
 * src/LindaPq/pglinda/src/LV_TYPE/pq_interface --
 *   
 */

#include <postgres.h>
// after postgres.h
#include <commands/trigger.h>
#include <fmgr.h>

#include "../../include/linda_funs.h"
#include "../../include/linda_type_funs.h"

// lv type string I/O
PG_FUNCTION_INFO_V1(linda_type_str_to_datum);
PG_FUNCTION_INFO_V1(datum_to_linda_type_str);

// lv type binary I/O
PG_FUNCTION_INFO_V1(linda_type_bytes_to_datum);
PG_FUNCTION_INFO_V1(datum_to_linda_type_bytes);

// utils
PG_FUNCTION_INFO_V1(linda_value_truetype);

// lv type comparisions
PG_FUNCTION_INFO_V1(linda_type_cmp);
PG_FUNCTION_INFO_V1(linda_type_lt);
PG_FUNCTION_INFO_V1(linda_type_le);
PG_FUNCTION_INFO_V1(linda_type_gt);
PG_FUNCTION_INFO_V1(linda_type_ge);
PG_FUNCTION_INFO_V1(linda_type_eq);
PG_FUNCTION_INFO_V1(linda_type_ne);

// lv type <-> lv comparisions
PG_FUNCTION_INFO_V1(linda_type_value_lt);
PG_FUNCTION_INFO_V1(linda_type_value_le);
PG_FUNCTION_INFO_V1(linda_type_value_gt);
PG_FUNCTION_INFO_V1(linda_type_value_ge);
PG_FUNCTION_INFO_V1(linda_type_value_eq);
PG_FUNCTION_INFO_V1(linda_type_value_ne);

// lv <-> lv type comparisions
PG_FUNCTION_INFO_V1(linda_value_type_lt);
PG_FUNCTION_INFO_V1(linda_value_type_le);
PG_FUNCTION_INFO_V1(linda_value_type_gt);
PG_FUNCTION_INFO_V1(linda_value_type_ge);
PG_FUNCTION_INFO_V1(linda_value_type_eq);
PG_FUNCTION_INFO_V1(linda_value_type_ne);

// string based I/O functions
Datum
linda_type_str_to_datum(PG_FUNCTION_ARGS) {
    char* str_raw = PG_GETARG_CSTRING(0);
    lv_type_internal internal = lv_type_str_to_internal(str_raw);
    PG_RETURN_DATUM(LvTypeGetDatum(internal));
}

Datum
datum_to_linda_type_str(PG_FUNCTION_ARGS) {
    Datum datum = PG_GETARG_DATUM(0);
    const char* str = internal_to_lv_type_str(DatumGetLvType(datum));
    PG_RETURN_CSTRING(str);
}

// byte based I/O functions
Datum
linda_type_bytes_to_datum(PG_FUNCTION_ARGS) {
    StringInfo buf = PG_GETARG_POINTER(0);
    lv_type_internal internal = lv_type_bytes_to_internal(buf);
    PG_RETURN_DATUM(LvTypeGetDatum(internal));
}

Datum
datum_to_linda_type_bytes(PG_FUNCTION_ARGS) {
    Datum datum = PG_GETARG_DATUM(0);
    lv_type_internal internal = DatumGetLvType(datum);
    bytea* bytes = internal_to_lv_type_bytes(internal);
    PG_RETURN_BYTEA_P(bytes);
}

// utils
Datum
linda_value_truetype(PG_FUNCTION_ARGS) {
    Datum ptr = PG_GETARG_DATUM(0);
    Pointer maybe_detoasted = PG_DETOAST_DATUM(ptr);
    lv_type_internal type = lv_truetype(maybe_detoasted);
    PG_FREE_IF_COPY(maybe_detoasted, 0);
    PG_RETURN_DATUM(LvTypeGetDatum(type));
}

// comparisions
Datum
linda_type_cmp(PG_FUNCTION_ARGS) {
    lv_type_internal left = LvTypeGetDatum(PG_GETARG_DATUM(0));
    lv_type_internal right = LvTypeGetDatum(PG_GETARG_DATUM(1));

    const int order = lv_type_cmp(left, right);

    PG_RETURN_INT32(order);
}

#define LV_TYPE_CMP_FN(name, op)                                     \
    Datum linda_type_##name(PG_FUNCTION_ARGS) {                      \
        lv_type_internal left = LvTypeGetDatum(PG_GETARG_DATUM(0));  \
        lv_type_internal right = LvTypeGetDatum(PG_GETARG_DATUM(1)); \
        PG_RETURN_BOOL((left op right));                             \
    }

#define LV_TYPE_VALUE_CMP_FN(name, impl)                            \
    Datum linda_type_value_##name(PG_FUNCTION_ARGS) {               \
        lv_type_internal left = LvTypeGetDatum(PG_GETARG_DATUM(0)); \
        Datum right = PG_GETARG_DATUM(1);                           \
        Pointer maybe_detoasted_right = PG_DETOAST_DATUM(right);    \
        const bool order = impl(left,                               \
                                maybe_detoasted_right);             \
        PG_FREE_IF_COPY(maybe_detoasted_right, 1);                  \
        PG_RETURN_BOOL(order);                                      \
    }

#define LV_VALUE_TYPE_CMP_FN(name, impl)                             \
    Datum linda_value_type_##name(PG_FUNCTION_ARGS) {                \
        Datum left = PG_GETARG_DATUM(0);                             \
        Pointer maybe_detoasted_left = PG_DETOAST_DATUM(left);       \
        lv_type_internal right = LvTypeGetDatum(PG_GETARG_DATUM(1)); \
        const bool order = impl(maybe_detoasted_left,                \
                                right);                              \
        PG_FREE_IF_COPY(maybe_detoasted_left, 0);                    \
        PG_RETURN_BOOL(order);                                       \
    }

LV_TYPE_CMP_FN(lt, <)
LV_TYPE_CMP_FN(le, <=)
LV_TYPE_CMP_FN(gt, >)
LV_TYPE_CMP_FN(ge, >=)
LV_TYPE_CMP_FN(eq, ==)
LV_TYPE_CMP_FN(ne, !=)

LV_TYPE_VALUE_CMP_FN(lt, lv_type_value_less)
LV_TYPE_VALUE_CMP_FN(le, lv_type_value_less_equal)
LV_TYPE_VALUE_CMP_FN(gt, lv_type_value_greater)
LV_TYPE_VALUE_CMP_FN(ge, lv_type_value_greater_equal)
LV_TYPE_VALUE_CMP_FN(eq, lv_type_value_equal)
LV_TYPE_VALUE_CMP_FN(ne, lv_type_value_inequal)

LV_VALUE_TYPE_CMP_FN(lt, lv_value_type_less)
LV_VALUE_TYPE_CMP_FN(le, lv_value_type_less_equal)
LV_VALUE_TYPE_CMP_FN(gt, lv_value_type_greater)
LV_VALUE_TYPE_CMP_FN(ge, lv_value_type_greater_equal)
LV_VALUE_TYPE_CMP_FN(eq, lv_value_type_equal)
LV_VALUE_TYPE_CMP_FN(ne, lv_value_type_inequal)
