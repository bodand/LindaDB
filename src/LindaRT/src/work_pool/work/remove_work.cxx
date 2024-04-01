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
 * Originally created: 2024-03-03.
 *
 * src/LindaRT/src/work_pool/work/remove_work --
 *   
 */


#include <chrono>
#include <thread>

#include <lrt/work_pool/work/remove_work.hxx>

using namespace std::literals;

void
lrt::remove_work::perform(const lrt::mpi_thread_context& context) {
    const auto tuple = deserialize(_bytes);
    mpi_thread_context::set_current(context);
    std::ofstream("_wp.log", std::ios::app) << "WORKING ON REMOVE(" << context.rank() << "): " << (*this) << ": " << tuple << "\n";
    auto op = _runtime->store().remove_nosignal(tuple);
    int commit_vote = static_cast<int>(op.has_value());
    int commit_consensus = 0;
    MPI_Request req = MPI_REQUEST_NULL;
    MPI_Iallreduce(&commit_vote,
                   &commit_consensus,
                   1,
                   MPI_INT,
                   MPI_LAND,
                   _status_response_comm,
                   &req);
    //            MPI_Wait(&req, MPI_STATUS_IGNORE);

    std::this_thread::sleep_for(50ns);

    int finished = false;
    int cancelled = false;
    MPI_Status stat{};
    for (int i = 0; i < 3; ++i) {
        MPI_Test(&req, &finished, &stat);
        if (finished) break;
        std::this_thread::sleep_for(1ms);
    }
    if (!finished) {
        //        MPI_Cancel(&req);
        std::ofstream("_await.log", std::ios::app) << "FAILED REDUCE ON COMM: " << std::hex << _status_response_comm << " RANK: " << context.rank() << "\n";
    }

    MPI_Test_cancelled(&stat, &cancelled);
    if (commit_consensus && finished && !cancelled) _runtime->store().insert_nosignal(tuple);

    if (commit_consensus) op->commit();
    std::ofstream("_wp.log", std::ios::app) << "WORKED ON REMOVE(" << context.rank() << "): " << (*this) << ": " << tuple << "\n";
}
