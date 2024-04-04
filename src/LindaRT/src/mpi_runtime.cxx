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
 * src/LindaRT/src/mpi_runtime --
 *   
 */

#include <algorithm>
#include <array>
#include <execution>
#include <fstream>
#include <limits>
#include <ranges>

#include <ldb/common.hxx>
#include <lrt/communication_tags.hxx>
#include <lrt/mpi_runtime.hxx>
#include <lrt/serialize/int.hxx>

#include <mpi.h>

lrt::mpi_runtime::mpi_runtime(int* argc, char*** argv) {
    if (_mpi_inited.test_and_set()) return;

    int got_thread = MPI_THREAD_SINGLE;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &got_thread);

    if (got_thread == MPI_THREAD_SINGLE
        || got_thread == MPI_THREAD_FUNNELED) throw incompatible_mpi_exception();

    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &_world_size);

    MPI_Comm_dup(MPI_COMM_WORLD, &_ack_world);
}

lrt::mpi_runtime::~mpi_runtime() {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}

namespace {
    void
    primitive_send(int to,
                   int tag,
                   MPI_Comm comm,
                   std::span<std::byte> payload) noexcept {
        assert_that(static_cast<std::size_t>(std::numeric_limits<int>::max()) >= payload.size(),
                    "error: trying to send larger payload than INT_MAX through MPI");
        MPI_Send(payload.data(),
                 static_cast<int>(payload.size()),
                 MPI_BYTE,
                 to,
                 tag,
                 comm);
    }

    std::pair<MPI_Status, std::vector<std::byte>>
    primitive_recv(int from,
                   int tag,
                   MPI_Comm comm) noexcept {
        MPI_Status stat; // NOLINT(*init*) MPI output param
        auto succ = MPI_Probe(from, tag, comm, &stat);
        assert_that(succ == MPI_SUCCESS);

        int buf_sz; // NOLINT(*init*) MPI output param
        succ = MPI_Get_count(&stat, MPI_BYTE, &buf_sz);
        assert_that(succ == MPI_SUCCESS);

        std::vector<std::byte> buf(buf_sz, std::byte{});
        succ = MPI_Recv(buf.data(),
                        buf_sz,
                        MPI_BYTE,
                        stat.MPI_SOURCE,
                        stat.MPI_TAG,
                        comm,
                        &stat);
        assert_that(succ == MPI_SUCCESS);

        return std::make_pair(stat, std::move(buf));
    }
}

void
lrt::mpi_runtime::send(int to, int tag, std::span<std::byte> payload) const {
    std::ignore = _rank; // make clang shut up about static methods
    primitive_send(to, tag, MPI_COMM_WORLD, payload);
}

void
lrt::mpi_runtime::send_ack(int to, int tag, std::span<std::byte> payload) const {
    std::ignore = _rank; // make clang shut up about static methods
    primitive_send(to, tag, _ack_world, payload);
}

namespace {
    template<std::constructible_from T>
    T
    deserialize_numeric(std::byte*& buf, std::size_t& len) {
        assert_that(len >= sizeof(T));
        T i{};
        auto value_representation =
               std::bit_cast<std::byte*>(&i);
        std::copy(std::execution::par_unseq,
                  buf,
                  buf + sizeof(T),
                  value_representation);
        len -= sizeof(T);
        buf += sizeof(T);
        if constexpr (std::integral<T>) {
            return lrt::from_communication_endian(i);
        }
        else {
            return i;
        }
    }
}

std::tuple<int, int, int, std::vector<std::byte>>
lrt::mpi_runtime::recv(int from, int tag) const {
    std::ignore = _rank; // make clang shut up about static methods

    auto [stat, acked_buf] = primitive_recv(from, tag, MPI_COMM_WORLD);
    auto* acked_buf_data = acked_buf.data();
    std::size_t acked_buf_sz = acked_buf.size();
    auto ack_tag = deserialize_numeric<int>(acked_buf_data, acked_buf_sz);
    std::ofstream("_msg.log", std::ios::app) << "rank " << _rank << " received " << acked_buf_sz - 4 << " bytes from " << stat.MPI_SOURCE << " with ack-tag " << std::hex << ack_tag << "\n";

    acked_buf.erase(acked_buf.begin(), std::next(acked_buf.begin(), sizeof(ack_tag)));

    return std::make_tuple(stat.MPI_SOURCE, stat.MPI_TAG, ack_tag, std::move(acked_buf));
}

namespace {
    template<std::input_iterator It, std::sentinel_for<It> S>
    std::size_t
    write_bytes_raw(It begin, S end, std::byte* buf) {
        const auto copied_end = std::copy(std::execution::par_unseq,
                                          begin,
                                          end,
                                          buf);
        return static_cast<std::size_t>(copied_end - buf);
    }

    template<std::integral T>
    std::size_t
    write_int_raw(T val, std::byte* buf) {
        auto value_representation =
               std::bit_cast<std::array<std::byte, sizeof(std::remove_cvref_t<T>)>>(lrt::to_communication_endian(val));
        return write_bytes_raw(value_representation.begin(), value_representation.end(), buf);
    }
}

int
lrt::mpi_runtime::send_with_ack(int to, int tag, std::span<std::byte> payload) const {
    std::ignore = _rank; // make clang shut up about static methods

    const auto ack_tag = make_ack_tag();
    std::vector<std::byte> ack_bytes(sizeof(ack_tag) + payload.size());

    // horrific copying :(
    auto* buf = ack_bytes.data();
    buf += write_int_raw(ack_tag, buf);
    write_bytes_raw(payload.begin(), payload.end(), buf);

    primitive_send(to, tag, MPI_COMM_WORLD, ack_bytes);
    return ack_tag;
}

std::vector<std::byte>
lrt::mpi_runtime::send_and_wait_ack(int to, int tag, std::span<std::byte> payload) const {
    const auto ack_tag = send_with_ack(to, tag, payload);
    std::ofstream("_msg.log", std::ios::app) << "rank " << rank() << " sending " << payload.size() << " bytes to " << to << " with tag " << std::hex << tag << " and ack tag " << ack_tag << "\n";
    auto [stat, recv_buf] = primitive_recv(to, ack_tag, _ack_world);
    std::ofstream("_msg.log", std::ios::app) << "rank " << rank() << "received " << recv_buf.size() << " bytes from " << to << " with tag " << std::hex << ack_tag << "\n";
    return recv_buf;
}
