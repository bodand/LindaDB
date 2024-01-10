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
#include <memory>
#include <utility>
#include <vector>

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
            MPI_Waitall(static_cast<int>(awaiter._reqs.size()),
                        awaiter._reqs.data(),
                        MPI_STATUS_IGNORE);
        }

        std::vector<MPI_Request> _reqs;
        std::unique_ptr<std::byte[]> _buf;
    };

    static_assert(ldb::awaitable<mpi_request_vector_awaiter>);

    struct bcast_handler {
        using await_type = mpi_request_vector_awaiter;

        explicit
        bcast_handler(const int rank, const int size)
             : _myrank(rank),
               _size(size) { }

    private:
        [[nodiscard]] std::vector<MPI_Request>
        broadcast_with_tag(int tag, std::span<std::byte> bytes) const {
            std::vector req(static_cast<std::size_t>(_size), MPI_REQUEST_NULL);
            std::vector<int> idx(static_cast<std::size_t>(_size));
            std::iota(idx.begin(), idx.end(), std::size_t{});
            std::transform(std::execution::par_unseq,
                           idx.begin(),
                           idx.end(),
                           req.begin(),
                           [bytes, tag, my_rank = _myrank](int rank) {
                               if (rank == my_rank) return MPI_REQUEST_NULL;

                               MPI_Request req;
                               const auto status = MPI_Isend(bytes.data(),
                                                             static_cast<int>(bytes.size()),
                                                             MPI_CHAR,
                                                             rank,
                                                             tag,
                                                             MPI_COMM_WORLD,
                                                             &req);
                               if (status != 0) {
                                   return MPI_REQUEST_NULL;
                               }
                               return req;
                           });

            return req;
        }

        friend mpi_request_vector_awaiter
        broadcast_insert(const bcast_handler& handler,
                         const ldb::lv::linda_tuple& tuple) {
            auto [val, val_sz] = lrt::serialize(tuple);
            auto reqs = handler.broadcast_with_tag(LINDA_RT_DB_SYNC_INSERT_TAG,
                                                   {val.get(), val_sz});
            return {std::move(reqs), std::move(val)};
        }

        friend mpi_request_vector_awaiter
        broadcast_delete(const bcast_handler& handler,
                         const ldb::lv::linda_tuple& tuple) {
            auto [val, val_sz] = lrt::serialize(tuple);
            auto reqs = handler.broadcast_with_tag(LINDA_RT_DB_SYNC_DELETE_TAG,
                                                   {val.get(), val_sz});
            return {std::move(reqs), std::move(val)};
        }

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
    _store.set_broadcast(bcast_handler(_rank, world_size));
    _recv_start.test_and_set();
    _recv_start.notify_all();
}

lrt::runtime::~
runtime() noexcept {
    constexpr char buf{'\0'};
    MPI_Send(&buf, 1, MPI_CHAR, _rank, LINDA_RT_TERMINATE_TAG, MPI_COMM_WORLD);
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
        if (len == 1) {
            char term_buf{};
            MPI_Recv(&term_buf, 1, MPI_CHAR, MPI_ANY_SOURCE, LINDA_RT_TERMINATE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (term_buf == 0) return;
            continue;
        }
        auto buf = std::make_unique<std::byte[]>(static_cast<std::size_t>(len));
        MPI_Recv(buf.get(), len, MPI_CHAR, MPI_ANY_SOURCE, LINDA_RT_DB_SYNC_INSERT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        auto rx_tuple = deserialize({buf.get(), static_cast<unsigned>(len)});
        _store.out_nosignal(rx_tuple);
    }
}
