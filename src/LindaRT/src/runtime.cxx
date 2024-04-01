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
#include <lrt/bcast/multi_thread_broadcast.hxx>
#include <lrt/bcast/manual_broadcast_handler.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/runtime.hxx>
#include <lrt/work_pool/work_factory.hxx>

#include <mpi.h>

lrt::runtime::runtime(int* argc,
                      char*** argv,
                      std::function<balancer(const runtime&)> load_balancer)
     : _mpi(argc, argv),
       _recv_thr(&lrt::runtime::recv_thread_worker, this),
       _work_pool([this]() {
           mpi_thread_context ctx{};
           MPI_Comm_dup(MPI_COMM_WORLD, &ctx.thread_communicator);
           std::ofstream("_comm.log", std::ios::app) << rank() << ": DUPING WORLD: " << std::hex << ctx.thread_communicator << std::endl;
           return std::make_tuple(ctx);
       }, 4) {
    mpi_thread_context main_ctx{};
    MPI_Comm_dup(MPI_COMM_WORLD, &main_ctx.thread_communicator);
    mpi_thread_context::set_current(main_ctx);
    std::ofstream("_comm.log", std::ios::app) << rank() << ": DUPING WORLD: " << std::hex << main_ctx.thread_communicator << std::endl;
    _main_thread_context = std::make_tuple(main_ctx);

    std::vector<std::tuple<mpi_thread_context>*> contexts;
    contexts.reserve(_work_pool.thread_contexts().size() + 1);
    contexts.push_back(&_main_thread_context);

    std::ranges::copy(_work_pool.thread_contexts(), std::back_inserter(contexts));

    {
        std::ofstream os("_comm.log", std::ios::app);
        os << rank() << ": " << std::hex;
        std::ranges::transform(contexts,
                               std::ostream_iterator<int>(os, " "),
                               [](std::tuple<mpi_thread_context>* ptr) {
                                   return std::get<0>(*ptr).thread_communicator;
                               });
        os << std::endl;
    }

    _broadcast = lrt::multi_thread_broadcast(contexts);
    _balancer = load_balancer(*this);

    _store.set_broadcast(_broadcast);
    _recv_start.test_and_set();
    _recv_start.notify_all();
}

lrt::runtime::~runtime() noexcept {
    std::ofstream("_runtime.log") << "RUNTIME ENDING 0: "
                                  << mpi_thread_context::current().thread_communicator << " =?= "
                                  << std::get<0>(_main_thread_context).thread_communicator << std::endl;
    await(broadcast_terminate(_broadcast));
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
    bool do_spin = true;
    while (do_spin) {
        auto msgs = broadcast_recv(_broadcast);
        for (auto& msg : msgs) {
            const auto command = static_cast<communication_tag>(msg.tag);
            std::ofstream("_cmd.log", std::ios::app) << rank() << ": RECEIVED COMMAND: " << std::hex << msg.tag << std::endl;

            if (command == communication_tag::Terminate) {
                do_spin = false;
                break;
            }
            auto work = lrt::work_factory::create(command,
                                                  std::move(msg.buffer),
                                                  *this,
                                                  msg.context);
            _work_pool.enqueue(std::move(work));
        }
    }
    _work_pool.terminate();
}

void
lrt::runtime::eval(const ldb::lv::linda_tuple& call_tuple) {
    assert_that(_balancer);
    const auto dest = _balancer->send_to_rank(call_tuple);
    await(send_eval(_broadcast, dest, call_tuple));
}

void
lrt::runtime::loop() {
    MPI_Barrier(MPI_COMM_WORLD);
}
