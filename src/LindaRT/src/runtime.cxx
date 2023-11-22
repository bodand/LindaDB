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
#include <bit>
#include <execution>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <lrt/runtime.hxx>
#include <lrt/serialize/tuple.hxx>

#include <mpi.h>

namespace {
    constexpr const int LINDA_RT_TERMINATE_TAG = 0xDB'00'01;
    constexpr const int LINDA_RT_DB_SYNC_TAG = 0xDB'00'02;

    struct bcast_awaiter {
        bcast_awaiter(std::vector<MPI_Request>&& reqs,
                      std::unique_ptr<std::byte[]>&& buf)
             : _reqs(std::move(reqs)),
               _buf(std::move(buf)) { }

        void
        operator()() {
            MPI_Waitall(static_cast<int>(_reqs.size()), _reqs.data(), MPI_STATUS_IGNORE);
        }

    private:
        std::vector<MPI_Request> _reqs;
        std::unique_ptr<std::byte[]> _buf;
    };

    struct bcast_handler {
        explicit
        bcast_handler(int rank, int size)
             : _myrank(rank),
               _size(size) { }

        ldb::move_only_function<void()>

        operator()(ldb::lv::linda_tuple tuple) const {
            std::vector<MPI_Request> req(static_cast<std::size_t>(_size), MPI_REQUEST_NULL);
            auto [val, val_sz] = lrt::serialize(tuple);
            int comm_size;
            MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
            SPDLOG_INFO("WORLD SIZE: {} INSTEAD OF {}", comm_size, _size);
            std::vector<int> idx(static_cast<std::size_t>(_size));
            std::iota(idx.begin(), idx.end(), std::size_t{});
            std::transform(std::execution::par_unseq,
                           idx.begin(),
                           idx.end(),
                           req.begin(),
                           [&val, val_sz, my_rank = _myrank](int rank) {
                               MPI_Request req;
                               if (rank == my_rank) return MPI_REQUEST_NULL;
                               auto status = MPI_Isend(val.get(),
                                                       static_cast<int>(val_sz),
                                                       MPI_CHAR,
                                                       rank,
                                                       LINDA_RT_DB_SYNC_TAG,
                                                       MPI_COMM_WORLD,
                                                       &req);
                               if (status != 0) {
                                   SPDLOG_ERROR("MPI_Isend: {}", status);
                                   return MPI_REQUEST_NULL;
                               }
                               return req;
                           });

            return bcast_awaiter(std::move(req), std::move(val));
        }

    private:
        int _myrank;
        int _size;
    };
}

lrt::runtime::
runtime(int* argc, char*** argv)
     : _recv_thr(&lrt::runtime::recv_thread_worker, this) {
    if (!_mpi_inited.test_and_set()) {
        int got_thread = MPI_THREAD_SINGLE;
        MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &got_thread);
        if (got_thread == MPI_THREAD_SINGLE
            || got_thread == MPI_THREAD_FUNNELED) {
            throw std::runtime_error("MPI_Init_thread: insufficient threading capabilities: "
                                     "LindaRT requires at least MPI_THREAD_SERIALIZED but the current "
                                     "MPI runtime cannot provide this functionality.");
        }
    }
    int world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    spdlog::set_pattern("%H:%M:%S.%f %l [%P] [%t] (%s:%#): %v");
    _store.set_broadcast(bcast_handler(_rank, world_size));
    _recv_start.test_and_set();
    _recv_start.notify_all();
}

lrt::runtime::~
runtime() noexcept {
    char buf{'\0'};
    MPI_Send(&buf, 1, MPI_CHAR, _rank, LINDA_RT_TERMINATE_TAG, MPI_COMM_WORLD);
    _recv_thr.join();
    SPDLOG_INFO("LRT FINALIZE");
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}

void
lrt::runtime::recv_thread_worker() {
    _recv_start.wait(false);
    for (;;) {
        MPI_Status stat{};
        SPDLOG_INFO("MPI PROBE");
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        int len;
        MPI_Get_count(&stat, MPI_CHAR, &len);
        SPDLOG_INFO("MPI PROBE FOUND {} CHARS", len);
        if (len == 1) {
            char term_buf{};
            SPDLOG_INFO("MPI RECV 1");
            MPI_Recv(&term_buf, 1, MPI_CHAR, MPI_ANY_SOURCE, LINDA_RT_TERMINATE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (term_buf == 0) {
                SPDLOG_INFO("MPI TERMINATE READER");
                return;
            }
            continue;
        }
        auto buf = std::make_unique<std::byte[]>(static_cast<std::size_t>(len));
        SPDLOG_INFO("MPI RECV PAYLOAD");
        MPI_Recv(buf.get(), len, MPI_CHAR, MPI_ANY_SOURCE, LINDA_RT_DB_SYNC_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        auto rx_tuple = deserialize({buf.get(), static_cast<unsigned>(len)});
        SPDLOG_INFO("LRT GOT {}", rx_tuple.dump_string());
        _store.out_nosignal(rx_tuple);
        SPDLOG_INFO("LRT STORED {}", rx_tuple.dump_string());
    }
}
