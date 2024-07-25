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

extern "C"
{
#include <postgres.h>
}

#include "../include/fam_datum.hxx"
#include "../include/linda_funs.h"

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
