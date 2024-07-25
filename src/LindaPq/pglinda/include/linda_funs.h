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
 * src/LindaPq/pqlinda/include/linda_funs --
 *   
 */
#ifndef LINDA_FUNS_H
#define LINDA_FUNS_H

#include <postgres.h>
// after postgres.h
#include <c.h>
#include <fmgr.h>

#ifdef __cplusplus
#  define LPQ_EXTERN extern "C"
#else
#  define LPQ_EXTERN
#endif

// string I/O
LPQ_EXTERN const char*
internal_to_lv_str(void* datum);

LPQ_EXTERN bytea*
internal_to_lv_bytes(void* datum);

// binary I/O
LPQ_EXTERN void*
lv_str_to_internal(char* str_raw);

LPQ_EXTERN void*
lv_bytes_to_internal(StringInfo buf);

// utils
LPQ_EXTERN int
lv_type(void* datum);

LPQ_EXTERN const char*
lv_nicetype(void* datum);

// comparisions
LPQ_EXTERN int
lv_cmp(void* lhs, void* rhs);

LPQ_EXTERN bool
lv_less(void* lhs, void* rhs);

LPQ_EXTERN bool
lv_less_equal(void* lhs, void* rhs);

LPQ_EXTERN bool
lv_greater(void* lhs, void* rhs);

LPQ_EXTERN bool
lv_greater_equal(void* lhs, void* rhs);

LPQ_EXTERN bool
lv_equal(void* lhs, void* rhs);

LPQ_EXTERN bool
lv_inequal(void* lhs, void* rhs);

LPQ_EXTERN int64
lv_hash(void* datum, int64 salt);

#endif
