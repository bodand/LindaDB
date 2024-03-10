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
 * src/LindaRT/src/bcast/manual_broadcast_handler --
 *   
 */

#include <execution>
#include <fstream>

#include <lrt/bcast/manual_broadcast_handler.hxx>

namespace {
    MPI_Request
    start_send_buffer_to_with_tag(const std::span<std::byte> buffer,
                                  int to_rank,
                                  int tag) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        auto tuple = lrt::deserialize(buffer);
        std::ofstream("_msglog.txt", std::ios::app) << rank << " -> " << to_rank << ": " << std::showbase << std::hex << tag
                                                    << std::dec << std::noshowbase << tuple << std::endl;

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
}

std::vector<MPI_Request>
lrt::manual_broadcast_handler::broadcast_with_tag(int tag, std::span<std::byte> bytes) const {
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
