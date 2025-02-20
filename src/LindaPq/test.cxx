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
 * Originally created: 2024-10-10.
 *
 * src/LindaPq/test --
 *   
 */

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query.hxx>
#include <lpq/db_context.hxx>
#include <lpq/db_notify_awaiter.hxx>
#include <lpq/db_query.hxx>
#include <lpq/query_tuple_to_sql.hxx>

#include <libpq-fe.h>
#include <mpi.h>

#include "lpq/pg_store.hxx"

int
main(int argc, char** argv) {
    using namespace ldb::lv::io;
    int g;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &g);

    lpq::pg_store store;

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const std::array tuples = {
        ldb::lv::linda_tuple(1, 2, 3),
        ldb::lv::linda_tuple(1, 2, "alma"),
        ldb::lv::linda_tuple(1, "alma", 3),
        ldb::lv::linda_tuple("alma", 2, 3),
    };


    if (rank != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(987));
        std::string given;
        if (const auto got = store.try_remove("Teszt", ldb::ref(&given))) {
            std::cout << "rank " << rank << " got a Teszt ember: " << *got << "\n";
        } else {
            std::cout << "rank " << rank << " did't get a Teszt ember\n";
        }
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(789));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla1"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla2"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla3"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla4"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla5"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla6"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla7"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla8"));
        store.insert(ldb::lv::linda_tuple("Teszt", "Béla9"));
    }

    MPI_Finalize();
}
