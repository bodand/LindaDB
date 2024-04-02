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
 * Originally created: 2024-03-02.
 *
 * src/LindaRT/src/work_pool/work_factory --
 *   
 */

#include <ldb/common.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/runtime.hxx>
#include <lrt/work_pool/work/eval_work.hxx>
#include <lrt/work_pool/work/insert_work.hxx>
#include <lrt/work_pool/work/nop_work.hxx>
#include <lrt/work_pool/work/remove_work.hxx>
#include <lrt/work_pool/work_factory.hxx>

lrt::work<>
lrt::work_factory::create(lrt::communication_tag tag,
                          std::vector<std::byte>&& payload,
                          lrt::runtime& runtime,
                          int sender,
                          int ack) {
    switch (tag) {
    case communication_tag::SyncInsert: return insert_work(std::move(payload), runtime, sender, ack);
    case communication_tag::SyncDelete: return remove_work(std::move(payload), runtime, sender, ack);
    case communication_tag::Eval: return eval_work(std::move(payload), runtime, sender, ack);
    case communication_tag::Terminate:
        // termination does not happen through this message system
        return nop_work{};
    }
    LDB_UNREACHABLE;
}
