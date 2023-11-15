/* LindaDB project
 *
 * Copyright (c) 2023 András Bodor <bodand@pm.me>
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
 * Originally created: 2023-10-20.
 *
 * src/LindaDB/src/lv/linda_tuple --
 *   Implements the functions of the linda_tuple type.
 */

#include <ldb/lv/linda_tuple.hxx>

ldb::lv::linda_value&
ldb::lv::linda_tuple::get_at(std::size_t idx) {
    assert(idx < _size);
    if (idx < 3) return _data_ref[idx];
    if (_size == 4 && idx == 3) return std::get<linda_value>(_tail);
    assert(_size > 4);
    return std::get<std::vector<linda_value>>(_tail)[idx - 3];
}

const ldb::lv::linda_value&
ldb::lv::linda_tuple::get_at(std::size_t idx) const {
    assert(idx < _size);
    if (idx < 3) return _data_ref[idx];
    if (_size == 4 && idx == 3) return std::get<linda_value>(_tail);
    assert(_size > 4);
    return std::get<std::vector<linda_value>>(_tail)[idx - 3];
}

std::ostream&
ldb::lv::operator<<(std::ostream& os, const ldb::lv::linda_tuple& tuple) {
    os << "(";
    for (std::size_t i = 0; i < tuple.size(); ++i) {
        os << tuple.get_at(i);
        if (i != tuple.size() - 1) os << ", ";
    }
    os << ")";
    return os;
}
