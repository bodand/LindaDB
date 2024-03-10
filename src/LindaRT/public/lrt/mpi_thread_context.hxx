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
 * src/LindaRT/public/lrt/mpi_thread_context --
 *   
 */
#ifndef LINDADB_MPI_THREAD_CONTEXT_HXX
#define LINDADB_MPI_THREAD_CONTEXT_HXX

#include <optional>

#include <mpi.h>

namespace lrt {
    struct mpi_thread_context {
        MPI_Comm thread_communicator;

        int
        rank() const noexcept {
            int rank;
            MPI_Comm_rank(thread_communicator, &rank);
            return rank;
        }

        int
        size() const noexcept {
            int size;
            MPI_Comm_size(thread_communicator, &size);
            return size;
        }

        static const mpi_thread_context&
        current() noexcept {
            if (current_context) return *current_context;
            return global_context;
        }

        static void
        set_current(const mpi_thread_context& context) noexcept {
            current_context = context;
        }

    private:
        static mpi_thread_context global_context;
        static thread_local std::optional<mpi_thread_context> current_context;
    };
}

#endif
