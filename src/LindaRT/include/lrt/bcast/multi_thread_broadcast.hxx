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
 * Originally created: 2024-03-08.
 *
 * src/LindaRT/include/lrt/bcast/multi_thread_broadcast --
 *   The multi_thread_broadcast is an implementation of the broadcast_if
 *   interface with each thread shouting through a different communicator
 *   on the MPI layer.
 */
#ifndef LINDADB_MULTI_THREAD_BROADCAST_HXX
#define LINDADB_MULTI_THREAD_BROADCAST_HXX

#include <ldb/bcast/broadcaster.hxx>
#include <ldb/bcast/null_broadcast.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/mpi_thread_context.hxx>

#include <mpi.h>

#include "mpi_request_vector_awaiter.hxx"

namespace lrt {
    struct multi_thread_broadcast {
        using context_type = MPI_Comm;

        explicit multi_thread_broadcast(std::span<std::tuple<mpi_thread_context>*> receivers);

    private:
        struct thread_local_receiver final {
            thread_local_receiver(MPI_Request* recv_req,
                                  std::tuple<mpi_thread_context>* context) noexcept;

            ldb::broadcast_msg<context_type>
            recv(int source);

        private:
            std::vector<std::byte>
            recv_payload(int size, int source);

            std::vector<std::byte>
            recv_sent_payload(int size, int source);

            std::vector<std::byte>
            recv_bcast_payload(int size, int source);

            std::tuple<mpi_thread_context>* _context;
            int _bcast_init_payload[2]{};
            //            std::array<int, 2> _bcast_init_payload{};
        };

        std::vector<ldb::broadcast_msg<context_type>>
        recv_any();

        std::vector<MPI_Request> _wait_handles;
        std::vector<thread_local_receiver> _receivers;

        friend std::vector<ldb::broadcast_msg<context_type>>
        broadcast_recv(multi_thread_broadcast& bcast) {
            return bcast.recv_any();
        }

        friend lrt::mpi_request_vector_awaiter
        broadcast_terminate(multi_thread_broadcast& bcast);

        friend ldb::null_awaiter<void>
        send_eval(multi_thread_broadcast& bcast, int to, const ldb::lv::linda_tuple& tuple);

        friend ldb::null_awaiter<bool>
        broadcast_insert(multi_thread_broadcast& bcast, const ldb::lv::linda_tuple& tuple);

        friend ldb::null_awaiter<bool>
        broadcast_delete(multi_thread_broadcast& bcast, const ldb::lv::linda_tuple& tuple);
    };
}

#endif
