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
 * src/LindaPq/pqlinda/src/pq_interface --
 *   
 */

#include <postgres.h>
// after postgres.h
#include <commands/trigger.h>
#include <fmgr.h>

#include "../../include/linda_funs.h"

PG_MODULE_MAGIC;

// string based I/O functions
PG_FUNCTION_INFO_V1(linda_value_str_to_datum);
PG_FUNCTION_INFO_V1(datum_to_linda_value_str);

// byte based I/O functions
PG_FUNCTION_INFO_V1(linda_value_bytes_to_datum);
PG_FUNCTION_INFO_V1(datum_to_linda_value_bytes);

// utilities
PG_FUNCTION_INFO_V1(linda_value_type);
PG_FUNCTION_INFO_V1(linda_value_nicetype);
PG_FUNCTION_INFO_V1(linda_notify_trigger);

// comparisions
PG_FUNCTION_INFO_V1(linda_value_cmp_equal_image);
PG_FUNCTION_INFO_V1(linda_value_cmp);
PG_FUNCTION_INFO_V1(linda_value_lt);
PG_FUNCTION_INFO_V1(linda_value_le);
PG_FUNCTION_INFO_V1(linda_value_gt);
PG_FUNCTION_INFO_V1(linda_value_ge);
PG_FUNCTION_INFO_V1(linda_value_eq);
PG_FUNCTION_INFO_V1(linda_value_ne);

// hashes
PG_FUNCTION_INFO_V1(linda_value_hash_32);
PG_FUNCTION_INFO_V1(linda_value_hash_64salt);

// string based I/O functions
Datum
linda_value_str_to_datum(PG_FUNCTION_ARGS) {
    void* internal = lv_str_to_internal(PG_GETARG_CSTRING(0));
    PG_RETURN_POINTER(internal);
}

Datum
datum_to_linda_value_str(PG_FUNCTION_ARGS) {
    Datum ptr = PG_GETARG_DATUM(0);
    Pointer maybe_detoasted = PG_DETOAST_DATUM(ptr);
    const char* str = internal_to_lv_str(maybe_detoasted);
    PG_FREE_IF_COPY(maybe_detoasted, 0);
    PG_RETURN_CSTRING(str);
}

// byte based I/O functions
Datum
linda_value_bytes_to_datum(PG_FUNCTION_ARGS) {
    StringInfo buf = PG_GETARG_POINTER(0);
    void* internal = lv_bytes_to_internal(buf);
    PG_RETURN_POINTER(internal);
}

Datum
datum_to_linda_value_bytes(PG_FUNCTION_ARGS) {
    Datum ptr = PG_GETARG_DATUM(0);
    Pointer maybe_detoasted = PG_DETOAST_DATUM(ptr);
    bytea* bytes = internal_to_lv_bytes(maybe_detoasted);
    PG_FREE_IF_COPY(maybe_detoasted, 0);
    PG_RETURN_BYTEA_P(bytes);
}

// utilities
Datum
linda_value_type(PG_FUNCTION_ARGS) {
    Datum ptr = PG_GETARG_DATUM(0);
    Pointer maybe_detoasted = PG_DETOAST_DATUM(ptr);
    int type = lv_type(maybe_detoasted);
    PG_FREE_IF_COPY(maybe_detoasted, 0);
    PG_RETURN_INT16(type);
}

Datum
linda_value_nicetype(PG_FUNCTION_ARGS) {
    Datum ptr = PG_GETARG_DATUM(0);
    Pointer maybe_detoasted = PG_DETOAST_DATUM(ptr);
    const char* nicetype = lv_nicetype(maybe_detoasted);
    PG_FREE_IF_COPY(maybe_detoasted, 0);
    PG_RETURN_CSTRING(nicetype);
}

Datum
linda_notify_trigger(PG_FUNCTION_ARGS) {
    if (!CALLED_AS_TRIGGER(fcinfo)) {
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_FUNCTION),
                 errmsg("function %s can only be called from triggers",
                        __func__)));
    }

    TriggerData* trigger_data = (TriggerData*) fcinfo->context;
    HeapTuple ret = trigger_data->tg_trigtuple;
    if (TRIGGER_FIRED_BY_UPDATE(trigger_data->tg_event)) ret = trigger_data->tg_newtuple;

    lv_notify_impl(ret, trigger_data->tg_relation);

    PG_RETURN_POINTER(ret);
}

// comparisions
Datum
linda_value_cmp(PG_FUNCTION_ARGS) {
    Datum left = PG_GETARG_DATUM(0);
    Datum right = PG_GETARG_DATUM(1);
    Pointer maybe_detoasted_left = PG_DETOAST_DATUM(left);
    Pointer maybe_detoasted_right = PG_DETOAST_DATUM(right);

    const int order = lv_cmp(maybe_detoasted_left,
                             maybe_detoasted_right);

    PG_FREE_IF_COPY(maybe_detoasted_left, 0);
    PG_FREE_IF_COPY(maybe_detoasted_right, 1);

    PG_RETURN_INT32(order);
}

#define LV_CMP_FN(name, impl)                                    \
    Datum linda_value_##name(PG_FUNCTION_ARGS) {                 \
        Datum left = PG_GETARG_DATUM(0);                         \
        Datum right = PG_GETARG_DATUM(1);                        \
        Pointer maybe_detoasted_left = PG_DETOAST_DATUM(left);   \
        Pointer maybe_detoasted_right = PG_DETOAST_DATUM(right); \
        const bool order = impl(maybe_detoasted_left,            \
                                maybe_detoasted_right);          \
        PG_FREE_IF_COPY(maybe_detoasted_left, 0);                \
        PG_FREE_IF_COPY(maybe_detoasted_right, 1);               \
        PG_RETURN_BOOL(order);                                   \
    }

LV_CMP_FN(lt, lv_less)
LV_CMP_FN(le, lv_less_equal)
LV_CMP_FN(gt, lv_greater)
LV_CMP_FN(ge, lv_greater_equal)
LV_CMP_FN(eq, lv_equal)
LV_CMP_FN(ne, lv_inequal)

Datum
linda_value_cmp_equal_image(PG_FUNCTION_ARGS) {
    PG_RETURN_BOOL(true);
}

// hashes
Datum
linda_value_hash_32(PG_FUNCTION_ARGS) {
    Datum ptr = PG_GETARG_DATUM(0);
    Pointer maybe_detoasted = PG_DETOAST_DATUM(ptr);

    const int64 hval = lv_hash(maybe_detoasted, 0);
    PG_FREE_IF_COPY(maybe_detoasted, 0);

    PG_RETURN_INT32(hval); // implicit cast to int32
}

Datum
linda_value_hash_64salt(PG_FUNCTION_ARGS) {
    Datum ptr = PG_GETARG_DATUM(0);
    Pointer maybe_detoasted = PG_DETOAST_DATUM(ptr);
    int64 salt = PG_GETARG_INT64(1);

    const int64 hval = lv_hash(maybe_detoasted, salt);
    PG_FREE_IF_COPY(maybe_detoasted, 0);

    PG_RETURN_INT64(hval);
}
