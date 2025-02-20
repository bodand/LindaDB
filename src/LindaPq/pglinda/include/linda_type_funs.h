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
 * src/LindaPq/pglinda/include/linda_type_funs --
 *   
 */
#ifndef LINDA_TYPE_FUNS_H
#define LINDA_TYPE_FUNS_H

#include <postgres.h>
// after postgres.h
#include <c.h>
#include <fmgr.h>

#ifdef __cplusplus
#  define LPQ_EXTERN extern "C"
#else
#  define LPQ_EXTERN
#endif

typedef uint8 lv_type_internal;
#define LvTypeGetDatum(x) UInt8GetDatum(x)
#define DatumGetLvType(x) DatumGetUInt8(x)

// string IO
LPQ_EXTERN const char*
internal_to_lv_type_str(lv_type_internal datum);

LPQ_EXTERN lv_type_internal
lv_type_str_to_internal(char* str_raw);

// binary IO
LPQ_EXTERN bytea*
internal_to_lv_type_bytes(lv_type_internal datum);

LPQ_EXTERN lv_type_internal
lv_type_bytes_to_internal(StringInfo buf);

// utils
LPQ_EXTERN lv_type_internal
lv_truetype(const void* datum);

// comparisions
// type <-> type (most type <-> type comps. are done in place at this time)
LPQ_EXTERN int
lv_type_cmp(lv_type_internal lhs, lv_type_internal rhs);

// type <-> value
LPQ_EXTERN int
lv_type_value_cmp(lv_type_internal lhs, void* rhs);

LPQ_EXTERN int
lv_type_value_less(lv_type_internal lhs, void* rhs);

LPQ_EXTERN int
lv_type_value_less_equal(lv_type_internal lhs, void* rhs);

LPQ_EXTERN int
lv_type_value_greater(lv_type_internal lhs, void* rhs);

LPQ_EXTERN int
lv_type_value_greater_equal(lv_type_internal lhs, void* rhs);

LPQ_EXTERN int
lv_type_value_equal(lv_type_internal lhs, void* rhs);

LPQ_EXTERN int
lv_type_value_inequal(lv_type_internal lhs, void* rhs);

// value <-> type
LPQ_EXTERN int
lv_value_type_cmp(void* lhs, lv_type_internal rhs);

LPQ_EXTERN int
lv_value_type_less(void* lhs, lv_type_internal rhs);

LPQ_EXTERN int
lv_value_type_less_equal(void* lhs, lv_type_internal rhs);

LPQ_EXTERN int
lv_value_type_greater(void* lhs, lv_type_internal rhs);

LPQ_EXTERN int
lv_value_type_greater_equal(void* lhs, lv_type_internal rhs);

LPQ_EXTERN int
lv_value_type_equal(void* lhs, lv_type_internal rhs);

LPQ_EXTERN int
lv_value_type_inequal(void* lhs, lv_type_internal rhs);


#endif
