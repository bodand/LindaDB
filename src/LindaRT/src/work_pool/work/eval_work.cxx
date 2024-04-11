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
 * Originally created: 2024-03-03.
 *
 * src/LindaRT/src/work_pool/work/eval_work --
 *   
 */

#include <ldb/lv/fn_call_holder.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <lrt/runtime.hxx>
#include <lrt/serialize/tuple.hxx>
#include <lrt/work_pool/work/eval_work.hxx>

#include <ldb/profile.hxx>

namespace {
    struct executing_transform {
        template<class T>
        ldb::lv::linda_value
        operator()(T&& val) const {
            return val;
        }

        ldb::lv::linda_value
        operator()(const ldb::lv::fn_call_holder& fn_call_holder) const {
            return fn_call_holder.execute()[0];
        }
    };
}

void
lrt::eval_work::perform() {
    LDBT_ZONE_A;
    const auto tuple = deserialize(_bytes);
    _runtime->ack(_sender, _ack_with); // eval ACK designates receiving the job

    std::vector<ldb::lv::linda_value> result_values;
    result_values.reserve(tuple.size());

    std::transform(tuple.begin(), tuple.end(), std::back_inserter(result_values), [](const ldb::lv::linda_value& lv) {
        return std::visit(executing_transform{}, lv);
    });


    const ldb::lv::linda_tuple& result_tuple = ldb::lv::linda_tuple(result_values);
    _runtime->out(result_tuple);
}

std::ostream&
lrt::operator<<(std::ostream& os, const lrt::eval_work& work) {
    std::ignore = work;
    return os << "[eval work] "
              << " on rank " << work._runtime->rank()
              << " on thread " << std::this_thread::get_id();
}
