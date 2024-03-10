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
 * Originally created: 2024-03-07.
 *
 * src/LindaRT/include/lrt/bcast/manual_broadcast_handler --
 *   
 */
#ifndef LINDADB_MANUAL_BROADCAST_HANDLER_HXX
#define LINDADB_MANUAL_BROADCAST_HANDLER_HXX

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <lrt/bcast/mpi_request_vector_awaiter.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/serialize/tuple.hxx>

#include <mpi.h>

namespace lrt {
    struct manual_broadcast_handler final {
        using await_type = lrt::mpi_request_vector_awaiter;

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
        broadcast_with_tag(int tag, std::span<std::byte> bytes) const;

        friend await_type
        broadcast_insert(const manual_broadcast_handler& handler,
                         const ldb::lv::linda_tuple& tuple) {
            auto [val, val_sz] = lrt::serialize(tuple);
            auto reqs = handler.broadcast_with_tag(static_cast<int>(lrt::communication_tag::SyncInsert),
                                                   {val.get(), val_sz});
            return {std::move(reqs), std::move(val)};
        }

        friend await_type
        broadcast_delete(const manual_broadcast_handler& handler,
                         const ldb::lv::linda_tuple& tuple) {
            auto [val, val_sz] = lrt::serialize(tuple);
            auto reqs = handler.broadcast_with_tag(static_cast<int>(lrt::communication_tag::SyncDelete),
                                                   {val.get(), val_sz});
            return {std::move(reqs), std::move(val)};
        }

        friend ldb::broadcast_msg
        broadcast_recv(const manual_broadcast_handler& handler) {
            MPI_Status stat{};
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

            int len;
            MPI_Get_count(&stat, MPI_CHAR, &len);
            auto buf = std::vector<std::byte>(static_cast<std::size_t>(len));

            MPI_Recv(buf.data(),
                     len,
                     MPI_CHAR,
                     stat.MPI_SOURCE,
                     stat.MPI_TAG,
                     MPI_COMM_WORLD,
                     &stat);

            return ldb::broadcast_msg{
                   .from = stat.MPI_SOURCE,
                   .tag = stat.MPI_TAG,
                   .buffer = std::move(buf)};
        }

        int _myrank;
        std::size_t _size;
    };
}

#endif
