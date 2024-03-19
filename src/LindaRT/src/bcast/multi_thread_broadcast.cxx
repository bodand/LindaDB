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
 * src/LindaRT/src/bcast/multi_thread_broadcast --
 *   
 */

#include <algorithm>
#include <fstream>
#include <ranges>

#include <lrt/bcast/multi_thread_broadcast.hxx>
#include <lrt/serialize/tuple.hxx>

namespace {
    constexpr const static auto RUNTIME_BCAST_INIT_TAG = 0xFF'00'01;

    template<class IndexType = std::size_t>
    struct zip_with_index_t {
        template<class T>
        auto
        operator()(T& val) {
            return std::make_pair(_index++, &val);
        }

    private:
        IndexType _index{};
    };
}

ldb::broadcast_msg<lrt::multi_thread_broadcast::context_type>
lrt::multi_thread_broadcast::thread_local_receiver::recv(int source) {
    auto payload = recv_payload(_bcast_init_payload[0], source);
    return ldb::broadcast_msg<MPI_Comm>{
           .from = source,
           .tag = _bcast_init_payload[1],
           .buffer = payload,
           .context = std::get<0>(*_context).thread_communicator};
}

std::vector<std::byte>
lrt::multi_thread_broadcast::thread_local_receiver::recv_payload(int size, int source) {
    if (size == std::size_t{0}) return {};
    if (_bcast_init_payload[1] == communication_tag::Eval) return recv_sent_payload(size, source);
    return recv_bcast_payload(size, source);
}

std::vector<std::byte>
lrt::multi_thread_broadcast::thread_local_receiver::recv_bcast_payload(int size, int source) {
    std::vector<std::byte> payload(static_cast<std::size_t>(size));
    MPI_Bcast(payload.data(),
              size,
              MPI_CHAR,
              source,
              std::get<0>(*_context).thread_communicator);
    return payload;
}

std::vector<std::byte>
lrt::multi_thread_broadcast::thread_local_receiver::recv_sent_payload(int size, int source) {
    std::vector<std::byte> payload(static_cast<std::size_t>(size));
    MPI_Recv(payload.data(),
             size,
             MPI_CHAR,
             source,
             MPI_ANY_TAG,
             std::get<0>(*_context).thread_communicator,
             MPI_STATUS_IGNORE);
    return payload;
}

lrt::multi_thread_broadcast::thread_local_receiver::thread_local_receiver(
       MPI_Request* recv_req,
       std::tuple<lrt::mpi_thread_context>* context) noexcept
     : _context(context) {
    std::ofstream("_comm.log", std::ios::app) << mpi_thread_context::current().rank() << ": MTBCAST THREAD RECEIVER CREATED: " << (void*) this << std::endl;
    MPI_Recv_init(_bcast_init_payload,
                  static_cast<int>(std::size(_bcast_init_payload)),
                  MPI_INT,
                  MPI_ANY_SOURCE,
                  RUNTIME_BCAST_INIT_TAG,
                  std::get<0>(*_context).thread_communicator,
                  recv_req);
    std::ofstream("_msg.log", std::ios::app) << mpi_thread_context::current().rank() << ": LISTENING"
                                             << " INTO BUFFER: " << (void*) _bcast_init_payload
                                             << " ON COMM: " << std::hex << std::get<0>(*_context).thread_communicator << std::endl;
}

lrt::multi_thread_broadcast::multi_thread_broadcast(std::span<std::tuple<mpi_thread_context>*> receivers)
     : _wait_handles(receivers.size(), MPI_REQUEST_NULL),
       _receivers() {
    _receivers.reserve(receivers.size());
    for (std::size_t i = 0; i < receivers.size(); ++i) {
        _receivers.emplace_back(&_wait_handles[i], receivers[i]);
    }
}

std::vector<ldb::broadcast_msg<typename lrt::multi_thread_broadcast::context_type>>
lrt::multi_thread_broadcast::recv_any() {
    MPI_Startall(static_cast<int>(_wait_handles.size()),
                 _wait_handles.data());

    int recv_index{};
    MPI_Status stat{};
    MPI_Waitany(static_cast<int>(_wait_handles.size()),
                _wait_handles.data(),
                &recv_index,
                &stat);

    std::vector<ldb::broadcast_msg<context_type>> recv_buffer;
    std::ofstream("_msg.log", std::ios::app) << mpi_thread_context::current().rank() << ": RECEIVING MESSAGE"
                                             << " FROM " << stat.MPI_SOURCE
                                             << std::endl;
    auto buf = _receivers[recv_index].recv(stat.MPI_SOURCE);
    std::ofstream("_msg.log", std::ios::app) << mpi_thread_context::current().rank() << ": RECEIVED MESSAGE"
                                             << " FROM " << stat.MPI_SOURCE
                                             << " :> " << std::hex << buf.tag
                                             << std::endl;
    recv_buffer.push_back(buf); //

    std::ranges::for_each(_wait_handles, //
                          [&recv_buffer, recv_index, this](auto zipped) {
                              auto& [idx, req] = zipped;
                              if (recv_index == idx) return;

                              int fin{};
                              MPI_Status stat;
                              MPI_Test(req, &fin, &stat);
                              if (fin) {
                                  std::ofstream("_msg.log", std::ios::app) << mpi_thread_context::current().rank() << ": RECEIVING MESSAGE"
                                                                           << " FROM " << stat.MPI_SOURCE
                                                                           << std::endl;
                                  auto buf = _receivers[idx].recv(stat.MPI_SOURCE);
                                  std::ofstream("_msg.log", std::ios::app) << mpi_thread_context::current().rank() << ": RECEIVED MESSAGE"
                                                                           << " FROM " << stat.MPI_SOURCE
                                                                           << " :> " << std::hex << buf.tag
                                                                           << std::endl;
                                  recv_buffer.push_back(buf);
                                  return;
                              }

                              MPI_Cancel(req);
                              MPI_Wait(req, MPI_STATUS_IGNORE); //
                          },
                          zip_with_index_t<int>{});

    return recv_buffer;
}

namespace {
    template<class T, MPI_Datatype MpiType>
    struct send_starter {
        explicit send_starter(std::span<T> payload, const lrt::mpi_thread_context& context)
             : payload(payload), context(context) { }

        MPI_Request
        operator()(int dest) noexcept {
            std::ofstream("_msg.log", std::ios::app) << context.rank() << " -> " << dest << ": SEND"
                                                     << " ON COMM: " << std::hex << context.thread_communicator
                                                     << std::endl;
            MPI_Request req;
            MPI_Isend(payload.data(),
                      static_cast<int>(payload.size()),
                      MpiType,
                      dest,
                      RUNTIME_BCAST_INIT_TAG,
                      context.thread_communicator,
                      &req);
            return req;
        }

        std::span<T> payload;
        const lrt::mpi_thread_context& context;
    };
}

lrt::mpi_request_vector_awaiter
lrt::broadcast_terminate(lrt::multi_thread_broadcast& /* dispatch tag */) {
    constexpr const static std::array<int, 2> meta_payload{0, static_cast<int>(communication_tag::Terminate)};

    auto& context = mpi_thread_context::current();
    std::ofstream("_msg.log", std::ios::app) << context.rank() << " -> *: TERM ON " << std::hex << context.thread_communicator << std::endl;

    std::vector<MPI_Request> reqs;
    reqs.reserve(static_cast<std::size_t>(context.size()));

    std::ranges::transform(
           std::views::iota(0, context.size())
                  | std::views::filter(std::bind_front(std::not_equal_to{}, context.rank())),
           std::back_inserter(reqs),
           send_starter<int, MPI_INT>(*const_cast<std::array<int, 2>*>(&meta_payload), context));

    return mpi_request_vector_awaiter(std::move(reqs), nullptr);
}

ldb::null_awaiter<void>
lrt::send_eval(lrt::multi_thread_broadcast& bcast, int to, const ldb::lv::linda_tuple& tuple) {
    auto& context = mpi_thread_context::current();
    auto [val, val_sz] = lrt::serialize(tuple);

    std::ofstream("_msg.log", std::ios::app) << context.rank() << " -> " << to << ": EVAL: " << tuple
                                             << " ON COMM: " << std::hex << context.thread_communicator
                                             << std::endl;

    std::array<int, 2> meta_payload{static_cast<int>(val_sz),
                                    static_cast<int>(communication_tag::Eval)};

    MPI_Send(meta_payload.data(),
             static_cast<int>(meta_payload.size()),
             MPI_INT,
             to,
             RUNTIME_BCAST_INIT_TAG,
             context.thread_communicator);

    MPI_Send(val.get(),
             static_cast<int>(val_sz),
             MPI_CHAR,
             to,
             static_cast<int>(communication_tag::Eval),
             context.thread_communicator);

    return ldb::null_awaiter<void>{};
}

ldb::null_awaiter<bool>
lrt::broadcast_insert(lrt::multi_thread_broadcast& bcast, const ldb::lv::linda_tuple& tuple) {
    auto [val, val_sz] = lrt::serialize(tuple);

    std::array<int, 2> meta_payload{static_cast<int>(val_sz),
                                    static_cast<int>(communication_tag::SyncInsert)};

    auto& context = mpi_thread_context::current();
    std::ofstream("_msg.log", std::ios::app) << context.rank() << " -> *: INSERT: " << tuple
                                             << " ON COMM: " << std::hex << context.thread_communicator
                                             << std::endl;

    std::vector<MPI_Request> reqs;
    reqs.reserve(static_cast<std::size_t>(context.size()));

    std::ranges::transform(
           std::views::iota(0, context.size())
                  | std::views::filter(std::bind_front(std::not_equal_to{}, context.rank())),
           std::back_inserter(reqs),
           send_starter<int, MPI_INT>(std::span(meta_payload), context));

    MPI_Waitall(static_cast<int>(reqs.size()),
                reqs.data(),
                MPI_STATUS_IGNORE);

    MPI_Bcast(val.get(),
              static_cast<int>(val_sz),
              MPI_CHAR,
              context.rank(),
              context.thread_communicator);

    int sender_yes = true; // initiator always agrees on commit
    int recv = false;      // abort by default
    MPI_Allreduce(&sender_yes,
                  &recv,
                  1,
                  MPI_INT,
                  MPI_LAND,
                  context.thread_communicator);

    return ldb::null_awaiter<bool>{};
}

lrt::reduce_awaiter
lrt::broadcast_delete(lrt::multi_thread_broadcast&, const ldb::lv::linda_tuple& tuple) {
    auto [val, val_sz] = lrt::serialize(tuple);

    std::array<int, 2> meta_payload{static_cast<int>(val_sz),
                                    static_cast<int>(communication_tag::SyncDelete)};

    const auto& context = mpi_thread_context::current();
    std::ofstream("_msg.log", std::ios::app) << context.rank() << " -> *: DELETE: " << tuple
                                             << " ON COMM: " << std::hex << context.thread_communicator
                                             << std::endl;

    std::vector<MPI_Request> reqs;
    reqs.reserve(static_cast<std::size_t>(context.size()));

    std::ranges::transform(
           std::views::iota(0, context.size())
                  | std::views::filter(std::bind_front(std::not_equal_to{}, context.rank())),
           std::back_inserter(reqs),
           send_starter<int, MPI_INT>(meta_payload, context));

    MPI_Waitall(static_cast<int>(reqs.size()),
                reqs.data(),
                MPI_STATUS_IGNORE);

    MPI_Bcast(val.get(),
              static_cast<int>(val_sz),
              MPI_CHAR,
              context.rank(),
              context.thread_communicator);

    return {context.thread_communicator, true};
}
