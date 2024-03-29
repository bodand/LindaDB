/* LindaDB project
 *
 * Copyright (c) 2023 András Bodor <bodand@pm.me>
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
 * Originally created: 2023-11-08.
 *
 * test/lindart --
 *   
 */

#include <exception>
#include <iostream>
#include <string>
#include <syncstream>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/match_type.hxx>
#include <lrt/runtime.hxx>

#include <mpi.h>

int
main(int argc, char** argv) try {
    lrt::runtime rt(&argc, &argv);
    auto& store = rt.store();

    int rank, size;
    std::string data;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        for (int i = 2; i <= size; ++i) {
            auto red = store.in("rank", i, ldb::ref(&data));
            std::osyncstream(std::cout) << "rank0: " << red << " from " << i << "\n"
                                        << std::flush;
        }
    }
    else {
        data = "Hello World!";
        ldb::lv::linda_tuple const tuple{"rank", rank + 1, data};
        store.out(tuple);
        std::osyncstream(std::cout) << "rank" << rank << ": finishing\n"
                                    << std::flush;
    }
} catch (const std::exception& ex) {
    std::cerr << "fatal: uncaught exception: " << ex.what() << "\n\n";
}