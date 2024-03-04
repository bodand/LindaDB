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
 * src/LindaRT/include/lrt/work_pool/work/insert_work --
 *   
 */
#ifndef LINDADB_INSERT_WORK_HXX
#define LINDADB_INSERT_WORK_HXX

#include <fstream>
#include <ostream>

#include <lrt/runtime.hxx>
#include <lrt/serialize/tuple.hxx>

#include <mpi.h>

namespace lrt {
    struct insert_work {
        explicit insert_work(std::span<std::byte> payload,
                             runtime& runtime)
             : _bytes(std::from_range, payload),
               _runtime(&runtime) { }

        void
        perform() {
            const auto tuple = deserialize(_bytes);
            int rank;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            std::ofstream("_log.txt", std::ios::app) << rank << ": INSERT " << tuple << std::endl;
            _runtime->store().out_nosignal(tuple);
        }

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const insert_work& work) {
            return os << "[insert work]";
        }

        std::vector<std::byte> _bytes;
        lrt::runtime* _runtime;
    };
}

#endif