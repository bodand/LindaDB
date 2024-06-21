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

#include "ldb/common.hxx"

#define LRT_NOINCLUDE_EVAL

#include <ldb/lv/linda_tuple.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/runtime.hxx>
#include <lrt/serialize/query.hxx>
#include <lrt/serialize/tuple.hxx>
#include <lrt/work_pool/work_factory.hxx>

#include <mpi.h>

namespace {
    static int rank = -1;
}

lrt::runtime::runtime(int* argc,
                      char*** argv,
                      std::function<balancer(const runtime&)> load_balancer)
     : _mpi(argc, argv),
       _recv_thr(&lrt::runtime::recv_thread_worker, this),
       _work_pool([]() { return std::make_tuple(); }, 8) {
    LDBT_ZONE_A;
    ::rank = _mpi.rank();
    _balancer = load_balancer(*this);

    _recv_start.test_and_set();
    _recv_start.notify_all();
}

lrt::runtime::~runtime() noexcept {
    LDBT_ZONE_A;
    if (_mpi.rank() == 0) MPI_Barrier(MPI_COMM_WORLD);
    std::ignore = _mpi.send_with_ack(_mpi.rank(),
                                     static_cast<int>(communication_tag::Terminate),
                                     std::span<std::byte>());
    _recv_thr.join();
    _work_pool.terminate();
    _work_pool.await_terminated();
    if (_mpi.rank() != 0) MPI_Barrier(MPI_COMM_WORLD);
}

void
lrt::runtime::recv_thread_worker() {
    LDBT_ZONE_A;
    _recv_start.wait(false);
    for (;;) {
        auto [from, tag, ack, payload] = _mpi.recv();
        const auto command = static_cast<communication_tag>(tag);

        if (command == communication_tag::Terminate) break;
        auto work = lrt::work_factory::create(command,
                                              std::move(payload),
                                              *this,
                                              from,
                                              ack,
                                              [&wp = this->_work_pool]<class W>(W&& work) {
                                                  wp.enqueue(std::forward<W>(work));
                                              });
        _work_pool.enqueue(std::move(work));
    }
    _work_pool.terminate();
}

void
lrt::runtime::eval(const ldb::lv::linda_tuple& call_tuple) {
    LDBT_ZONE_A;
    assert_that(_balancer);
    const auto dest = _balancer->send_to_rank(call_tuple);
    const auto [buf, buf_sz] = serialize(call_tuple);
    std::ignore = _mpi.send_and_wait_ack(dest,
                                         static_cast<int>(communication_tag::Eval),
                                         std::span(buf.get(), buf_sz));
}

void
lrt::runtime::loop() {
    LDBT_ZONE_A;
    assert_that(_mpi.world_size() > 0);
    MPI_Barrier(MPI_COMM_WORLD);
}

void
lrt::runtime::ack(int to, int ack, std::span<std::byte> data) {
    LDBT_ZONE_A;
    //    std::ofstream("_eval.log", std::ios::app) << "sending ack from " << rank()
    //                                              << " to " << to
    //                                              << ": " << std::hex << ack << std::dec
    //                                              << " with payload of " << data.size() << " bytes\n";
    _mpi.send_ack(to, ack, data);
}
void
lrt::runtime::remote_insert(const ldb::lv::linda_tuple& tuple) {
    LDBT_ZONE_A;
    const auto [buf, buf_sz] = serialize(tuple);
    std::ignore = _mpi.send_and_wait_ack(0,
                                         static_cast<int>(communication_tag::Insert),
                                         std::span(buf.get(), buf_sz));
}

namespace {
    std::vector<std::byte>
    send_query_with_tag(lrt::mpi_runtime& mpi,
                        const ldb::tuple_query<ldb::simple_store::storage_type>& query,
                        lrt::communication_tag tag) {
        LDBT_ZONE_A;
        const auto [buf, buf_sz] = lrt::serialize(query);
        return mpi.send_and_wait_ack(0,
                                     static_cast<int>(tag),
                                     std::span(buf.get(), buf_sz));
    }

    auto
    insert_response_into_query_fields(std::span<const std::byte> tuple_bytes,
                                      const ldb::tuple_query<ldb::simple_store::storage_type>& query) {
        LDBT_ZONE_A;
        const auto result_tuple = lrt::deserialize(tuple_bytes);
        // insert values into query fields by performing an imitated search
        const auto cmp_res = result_tuple <=> query;
        assert_that(std::is_eq(cmp_res), "received tuple from remote does not match query");
        return result_tuple;
    }
}

bool
lrt::runtime::remote_try_remove(const ldb::tuple_query<ldb::simple_store::storage_type>& query) {
    LDBT_ZONE_A;
    const auto response = send_query_with_tag(_mpi, query, communication_tag::TryDelete);
    if (response.empty()) return false;
    insert_response_into_query_fields(std::span(response), query);
    return true;
}

ldb::lv::linda_tuple
lrt::runtime::remote_remove(const ldb::tuple_query<ldb::simple_store::storage_type>& query) {
    LDBT_ZONE_A;
    const auto response = send_query_with_tag(_mpi, query, communication_tag::Delete);
    assert_that(!response.empty(), "result of blocking remote db call empty");
    return insert_response_into_query_fields(std::span(response), query);
}

bool
lrt::runtime::remote_try_read(const ldb::tuple_query<ldb::simple_store::storage_type>& query) {
    LDBT_ZONE_A;
    const auto response = send_query_with_tag(_mpi, query, communication_tag::TrySearch);
    if (response.empty()) return false;
    insert_response_into_query_fields(std::span(response), query);
    return true;
}

ldb::lv::linda_tuple
lrt::runtime::remote_read(const ldb::tuple_query<ldb::simple_store::storage_type>& query) {
    LDBT_ZONE_A;
    const auto response = send_query_with_tag(_mpi, query, communication_tag::Search);
    assert_that(!response.empty(), "result of blocking remote db call empty");
    return insert_response_into_query_fields(std::span(response), query);
}
