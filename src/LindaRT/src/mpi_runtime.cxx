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

#include <fstream>

#include <lrt/mpi_runtime.hxx>

#include <mpi.h>

lrt::mpi_runtime::mpi_runtime(int* argc, char*** argv) {
    if (_mpi_inited.test_and_set()) return;

    std::ofstream("_cmd.log", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_comm.log", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_fa.log", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_log.txt", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_msg.log", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_recv.log", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_runtime.log", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_term.log", std::ios::app) << "----------------\n" << std::endl;
    std::ofstream("_wp.log", std::ios::app) << "----------------\n" << std::endl;

    int got_thread = MPI_THREAD_SINGLE;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &got_thread);

    if (got_thread == MPI_THREAD_SINGLE
        || got_thread == MPI_THREAD_FUNNELED) throw incompatible_mpi_exception();

    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &_world_size);
}

lrt::mpi_runtime::~mpi_runtime() {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}
