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
 * src/LindaPq/pglinda/src/LV_TYPE/linda_type_compare --
 *   
 */


#include "../../include/fam_datum.hxx"
#include "../../include/linda_type_funs.h"

namespace {
    constexpr int
    ordering_to_int(std::strong_ordering o) noexcept {
        if (o < 0) return -1;
        if (o > 0) return 1;
        return 0;
    }
}

int
lv_type_cmp(lv_type_internal lhs, lv_type_internal rhs) {
    return ordering_to_int(lhs <=> rhs);
}

#define LEFT_TYPE (static_cast<const fam_datum*>(lhs)->type())
#define RIGHT_TYPE (static_cast<const fam_datum*>(rhs)->type())

int
lv_type_value_cmp(lv_type_internal lhs, void* rhs) {
    return ordering_to_int(lhs <=> RIGHT_TYPE);
}

int
lv_value_type_cmp(void* lhs, lv_type_internal rhs) {
    return ordering_to_int(LEFT_TYPE <=> rhs);
}

#define TYPE_VALUE_CMP_FN(name, op)                                  \
    int lv_type_value_##name(lv_type_internal lhs, void* rhs) {      \
        return lhs op RIGHT_TYPE;                                    \
    }

#define VALUE_TYPE_CMP_FN(name, op)                                 \
    int lv_value_type_##name(void* lhs, lv_type_internal rhs) {     \
        return LEFT_TYPE op rhs;                                    \
    }

TYPE_VALUE_CMP_FN(less, <)
TYPE_VALUE_CMP_FN(less_equal, <=)
TYPE_VALUE_CMP_FN(greater, >)
TYPE_VALUE_CMP_FN(greater_equal, >=)
TYPE_VALUE_CMP_FN(equal, ==)
TYPE_VALUE_CMP_FN(inequal, !=)

VALUE_TYPE_CMP_FN(less, <)
VALUE_TYPE_CMP_FN(less_equal, <=)
VALUE_TYPE_CMP_FN(greater, >)
VALUE_TYPE_CMP_FN(greater_equal, >=)
VALUE_TYPE_CMP_FN(equal, ==)
VALUE_TYPE_CMP_FN(inequal, !=)
