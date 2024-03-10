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
 * src/LindaRT/include/lrt/bcast/mpi_request_vector_awaiter --
 *   
 */
#ifndef LINDADB_MPI_REQUEST_VECTOR_AWAITER_HXX
#define LINDADB_MPI_REQUEST_VECTOR_AWAITER_HXX

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <ldb/bcast/broadcaster.hxx>
#include <ldb/common.hxx>

#include <mpi.h>

namespace lrt {
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

    static_assert(ldb::await_if<mpi_request_vector_awaiter>);
}

#endif
