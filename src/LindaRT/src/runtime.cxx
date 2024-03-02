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
#include <ios>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <syncstream>
#include <utility>
#include <vector>

#include <ldb/bcast/broadcaster.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <lrt/runtime.hxx>
#include <lrt/serialize/tuple.hxx>

#include <mpi.h>

namespace {
    constexpr const int LINDA_RT_TERMINATE_TAG = 0xDB'00'01;
    constexpr const int LINDA_RT_DB_SYNC_INSERT_TAG = 0xDB'00'02;
    constexpr const int LINDA_RT_DB_SYNC_DELETE_TAG = 0xDB'00'03;

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
            auto reqs = handler.broadcast_with_tag(LINDA_RT_DB_SYNC_INSERT_TAG,
                                                   {val.get(), val_sz});
            return {std::move(reqs), std::move(val)};
        }

        friend mpi_request_vector_awaiter
        broadcast_delete(const manual_broadcast_handler& handler,
                         const ldb::lv::linda_tuple& tuple) {
            auto [val, val_sz] = lrt::serialize(tuple);
            auto reqs = handler.broadcast_with_tag(LINDA_RT_DB_SYNC_DELETE_TAG,
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

lrt::runtime::runtime(int* argc, char*** argv)
     : _recv_thr(&lrt::runtime::recv_thread_worker, this) {
    if (!_mpi_inited.test_and_set()) init_threaded_mpi(argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    _store.set_broadcast(manual_broadcast_handler::for_communicator(MPI_COMM_WORLD));
    _recv_start.test_and_set();
    _recv_start.notify_all();
}

lrt::runtime::~runtime() noexcept {
    constexpr char buf{};
    MPI_Send(&buf, 0, MPI_CHAR, _rank, LINDA_RT_TERMINATE_TAG, MPI_COMM_WORLD);
    _recv_thr.join();
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
        const auto command = stat.MPI_TAG;
        const auto payload = std::span<std::byte>(buf);

        switch (command) {
        case LINDA_RT_DB_SYNC_INSERT_TAG: {
            const auto rx_inserted = deserialize(payload);
            std::osyncstream(std::cout) << "INSERT (" << stat.MPI_SOURCE << " -> " << _rank << "): " << rx_inserted << "\n";
            _store.out_nosignal(rx_inserted);
            break;
        }

        case LINDA_RT_DB_SYNC_DELETE_TAG: {
            const auto rx_deleted = deserialize(payload);
            std::osyncstream(std::cout) << "REMOVE (" << stat.MPI_SOURCE << " -> " << _rank << "): " << rx_deleted << "\n";
            _store.remove_nosignal(rx_deleted);
            break;
        }

        case LINDA_RT_TERMINATE_TAG:
            return;

        default:
            std::cerr << "ERROR: unknown command received ("
                      << std::showbase << std::hex << command << std::dec << std::noshowbase
                      << ")\n";
        }
    }
}
