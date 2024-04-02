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
 * src/LindaRT/public/mpi_runtime --
 *   
 */
#ifndef LINDADB_MPI_RUNTIME_HXX
#define LINDADB_MPI_RUNTIME_HXX

#include <atomic>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>

#include <mpi.h>

namespace lrt {
    struct incompatible_mpi_exception : std::runtime_error {
        incompatible_mpi_exception()
             : std::runtime_error("MPI_Init_thread: insufficient threading capabilities: "
                                  "LindaRT requires at least MPI_THREAD_SERIALIZED but the current "
                                  "MPI runtime cannot provide this functionality.") { }
    };

    struct mpi_runtime {
        mpi_runtime(int* argc, char*** argv);

        mpi_runtime(const mpi_runtime& cp) = delete;
        mpi_runtime&
        operator=(const mpi_runtime& cp) = delete;

        mpi_runtime(mpi_runtime&& mv) noexcept = default;
        mpi_runtime&
        operator=(mpi_runtime&& mv) noexcept = default;

        ~mpi_runtime();

        void
        send(int to, int tag, std::span<std::byte> payload) const;

        void
        send_ack(int to, int tag, std::span<std::byte> payload) const;

        [[nodiscard]] std::tuple<int, int, int, std::vector<std::byte>>
        recv(int from = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG) const;

        [[nodiscard]] int
        send_with_ack(int to, int tag, std::span<std::byte> payload) const;

        [[nodiscard]] std::vector<std::byte>
        send_and_wait_ack(int to, int tag, std::span<std::byte> payload) const;

        [[nodiscard]] int
        rank() const noexcept { return _rank; }

        [[nodiscard]] int
        world_size() const noexcept { return _world_size; }
    private:

        inline static std::atomic_flag _mpi_inited = ATOMIC_FLAG_INIT;
        int _rank;
        int _world_size;
        MPI_Comm _ack_world;
    };
}

#endif
