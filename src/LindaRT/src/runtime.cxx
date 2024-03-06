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
#include <cstddef>
#include <execution>
#include <fstream>
#include <ios>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#define LRT_NOINCLUDE_EVAL

#include <ldb/lv/linda_tuple.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/runtime.hxx>
#include <lrt/serialize/tuple.hxx>
#include <lrt/work_pool/work_factory.hxx>

#include <mpi.h>

namespace {
    struct mpi_request_vector_awaiter final {
        mpi_request_vector_awaiter(std::vector<MPI_Request>&& reqs,
                                   std::unique_ptr<std::byte[]>&& buf)
             : _reqs(std::move(reqs)),
               _buf(std::move(buf)) { }

    private:
        friend void
        await(mpi_request_vector_awaiter& awaiter) {
            assert_that(!awaiter._finished);
            MPI_Waitall(static_cast<int>(awaiter._reqs.size()),
                        awaiter._reqs.data(),
                        MPI_STATUS_IGNORE);
            awaiter._finished = true;
            awaiter._buf.reset();
        }

        bool _finished{};
        std::vector<MPI_Request> _reqs;
        std::unique_ptr<std::byte[]> _buf;
    };

    static_assert(ldb::awaitable<mpi_request_vector_awaiter>);

    MPI_Request
    start_send_buffer_to_with_tag(const std::span<std::byte> buffer,
                                  int to_rank,
                                  int tag) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        auto tuple = lrt::deserialize(buffer);
        std::ofstream("_msglog.txt", std::ios::app) << rank << " -> " << to_rank << ": " << std::showbase << std::hex << tag
                                                    << std::dec << std::noshowbase << tuple << std::endl;

        auto req = MPI_REQUEST_NULL;
        if (const auto status = MPI_Isend(buffer.data(),
                                          static_cast<int>(buffer.size()),
                                          MPI_CHAR,
                                          to_rank,
                                          tag,
                                          MPI_COMM_WORLD,
                                          &req);
            status != 0) return MPI_REQUEST_NULL;
        return req;
    }

    struct manual_broadcast_handler final {
        using await_type = mpi_request_vector_awaiter;

        static manual_broadcast_handler
        for_communicator(MPI_Comm comm) {
            int rank;
            int comm_size;
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &comm_size);
            return {rank, comm_size};
        }

    private:
        manual_broadcast_handler(const int rank, const int size)
             : _myrank(rank),
               _size(static_cast<std::size_t>(size)) { }

        [[nodiscard]] std::vector<int>
        get_recipients() const {
            std::vector<int> recipient_ranks(_size - 1);
            std::ranges::generate(recipient_ranks, [idx = 0, skipped = _myrank]() mutable {
                const auto rank = idx++;
                if (rank == skipped) return idx++;
                return rank;
            });
            return recipient_ranks;
        }

        [[nodiscard]] std::vector<MPI_Request>
        broadcast_with_tag(int tag, std::span<std::byte> bytes) const {
            std::vector requests(_size, MPI_REQUEST_NULL);
            const auto recipient_ranks = get_recipients();
            std::transform(std::execution::par_unseq,
                           recipient_ranks.begin(),
                           recipient_ranks.end(),
                           requests.begin(),
                           [bytes, tag](int rank) {
                               return start_send_buffer_to_with_tag(bytes, rank, tag);
                           });

            return requests;
        }

        friend mpi_request_vector_awaiter
        broadcast_insert(const manual_broadcast_handler& handler,
                         const ldb::lv::linda_tuple& tuple) {
            auto [val, val_sz] = lrt::serialize(tuple);
            auto reqs = handler.broadcast_with_tag(static_cast<int>(lrt::communication_tag::SyncInsert),
                                                   {val.get(), val_sz});
            return {std::move(reqs), std::move(val)};
        }

        friend mpi_request_vector_awaiter
        broadcast_delete(const manual_broadcast_handler& handler,
                         const ldb::lv::linda_tuple& tuple) {
            auto [val, val_sz] = lrt::serialize(tuple);
            auto reqs = handler.broadcast_with_tag(static_cast<int>(lrt::communication_tag::SyncDelete),
                                                   {val.get(), val_sz});
            return {std::move(reqs), std::move(val)};
        }

        int _myrank;
        std::size_t _size;
    };

    struct incompatible_mpi_exception : std::runtime_error {
        incompatible_mpi_exception()
             : std::runtime_error("MPI_Init_thread: insufficient threading capabilities: "
                                  "LindaRT requires at least MPI_THREAD_SERIALIZED but the current "
                                  "MPI runtime cannot provide this functionality.") { }
    };

    void
    init_threaded_mpi(int* argc, char*** argv) {
        int got_thread = MPI_THREAD_SINGLE;
        MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &got_thread);

        if (got_thread == MPI_THREAD_SINGLE
            || got_thread == MPI_THREAD_FUNNELED) throw incompatible_mpi_exception();
    }
}

lrt::runtime::runtime(int* argc, char*** argv, std::function<balancer(const runtime&)> load_balancer)
     : _recv_thr(&lrt::runtime::recv_thread_worker, this),
       _work_pool(1) {
    if (!_mpi_inited.test_and_set()) init_threaded_mpi(argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &_world_size);

    _balancer = load_balancer(*this);

    _store.set_broadcast(manual_broadcast_handler::for_communicator(MPI_COMM_WORLD));
    _recv_start.test_and_set();
    _recv_start.notify_all();
}

lrt::runtime::~runtime() noexcept {
    constexpr char buf{};
    MPI_Send(&buf, 0, MPI_CHAR, _rank, static_cast<int>(lrt::communication_tag::Terminate), MPI_COMM_WORLD);
    _recv_thr.join();
    _work_pool.terminate();
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}

void
lrt::runtime::recv_thread_worker() {
    _recv_start.wait(false);
    for (;;) {
        MPI_Status stat{};
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

        int len;
        MPI_Get_count(&stat, MPI_CHAR, &len);
        auto buf = std::vector<std::byte>(static_cast<std::size_t>(len));

        MPI_Recv(buf.data(), len, MPI_CHAR, stat.MPI_SOURCE, stat.MPI_TAG, MPI_COMM_WORLD, &stat);
        const auto command = static_cast<communication_tag>(stat.MPI_TAG);
        const auto payload = std::span<std::byte>(buf);

        if (command == communication_tag::Terminate) break;
        auto work = lrt::work_factory::create(command, payload, *this);
        _work_pool.enqueue(std::move(work));
    }
    _work_pool.terminate();
}

void
lrt::runtime::eval(const ldb::lv::linda_tuple& call_tuple) {
    assert_that(_balancer);
    const auto dest = _balancer->send_to_rank(call_tuple);
    const auto [rx_buf, rx_buf_sz] = serialize(call_tuple);

    MPI_Send(rx_buf.get(),
             static_cast<int>(rx_buf_sz),
             MPI_CHAR,
             dest,
             static_cast<int>(lrt::communication_tag::Eval),
             MPI_COMM_WORLD);
}

void
lrt::runtime::loop() {
    MPI_Barrier(MPI_COMM_WORLD);
}
