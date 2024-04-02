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
 * Originally created: 2023-11-06.
 *
 * src/LindaRT/src/runtime --
 *   Implementation of the runtime object of LindaRT.
 */

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>
#include <utility>

#define LRT_NOINCLUDE_EVAL

#include <ldb/lv/linda_tuple.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/runtime.hxx>
#include <lrt/work_pool/work_factory.hxx>

#include <mpi.h>

#include "lrt/serialize/tuple.hxx"

lrt::runtime::runtime(int* argc,
                      char*** argv,
                      std::function<balancer(const runtime&)> load_balancer)
     : _mpi(argc, argv),
       _recv_thr(&lrt::runtime::recv_thread_worker, this),
       _work_pool([]() { return std::make_tuple(); },
                  4) {
    _balancer = load_balancer(*this);

    _recv_start.test_and_set();
    _recv_start.notify_all();
}

lrt::runtime::~runtime() noexcept {
    std::ignore = _mpi.send_with_ack(_mpi.rank(),
                                     static_cast<int>(communication_tag::Terminate),
                                     std::span<std::byte>());
    std::ofstream("_runtime.log") << "RUNTIME ENDING 1" << std::endl;
    _recv_thr.join();
    std::ofstream("_runtime.log") << "RUNTIME ENDING 2" << std::endl;
    _store.terminate();
    std::ofstream("_runtime.log") << "RUNTIME ENDING 3" << std::endl;
    _work_pool.terminate();
    std::ofstream("_runtime.log") << "RUNTIME ENDING 4" << std::endl;
}

void
lrt::runtime::recv_thread_worker() {
    _recv_start.wait(false);
    for (;;) {
        auto [from, tag, ack, payload] = _mpi.recv();
        const auto command = static_cast<communication_tag>(tag);

        if (command == communication_tag::Terminate) break;
        auto work = lrt::work_factory::create(command,
                                              std::move(payload),
                                              *this,
                                              from,
                                              0);
        _work_pool.enqueue(std::move(work));
    }
    _work_pool.terminate();
}

void
lrt::runtime::eval(const ldb::lv::linda_tuple& call_tuple) {
    assert_that(_balancer);
    const auto dest = _balancer->send_to_rank(call_tuple);
    const auto [buf, buf_sz] = serialize(call_tuple);
    std::ignore = _mpi.send_and_wait_ack(dest,
                                         static_cast<int>(communication_tag::Eval),
                                         std::span(buf.get(), buf_sz));
}

void
lrt::runtime::loop() {
    assert_that(_mpi.world_size() > 0);
    MPI_Barrier(MPI_COMM_WORLD);
}

void
lrt::runtime::ack(int to, int ack, std::span<std::byte> data) {
    _mpi.send_ack(to, ack, data);
}
