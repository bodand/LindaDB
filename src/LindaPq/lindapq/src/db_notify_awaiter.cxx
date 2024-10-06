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
 * src/LindaPq/lindapq/src/db_notify_awaiter --
 *   
 */

#include <cstring>
#include <iostream>

#include <lpq/db_context.hxx>
#include <lpq/db_notify_awaiter.hxx>

#include <libpq-fe.h>

namespace {
    struct result_freer {
        void
        operator()(PGresult* res) const noexcept {
            PQclear(res);
        }
    };

    struct pgmem_freer {
        void
        operator()(void* notify) const noexcept {
            PQfreemem(notify);
        }
    };


    using unique_result = std::unique_ptr<PGresult, result_freer>;
    using unique_notify = std::unique_ptr<PGnotify, pgmem_freer>;
}

lpq::db_notify_awaiter::
db_notify_awaiter(db_context& db, std::string_view channel) : _db(db) {
    const auto sql = std::format("LISTEN {}", channel);
    const auto conn = static_cast<PGconn*>(db.native_handle());

    const unique_result res(PQexec(conn, sql.c_str()));
    if (PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
        const auto postgres_error = PQerrorMessage(conn);
        std::cerr << "fatal: cannot listen on '" << channel << "' channel: "
                  << postgres_error
                  << "\n";
        throw std::runtime_error(postgres_error);
    }

    _socket = PQsocket(conn);
}

void
lpq::db_notify_awaiter::loop() {
    namespace chr = std::chrono;
    const pg_usec_time_t timeout = chr::duration_cast<chr::microseconds>(cfg_loop_timeout).count();
    const auto conn = static_cast<PGconn*>(_db.native_handle());

    while (!_stop.test()) {
        if (PQsocketPoll(_socket, 1, 0, timeout) < 0) {
            auto sys_error = strerror(errno);
            std::cerr << "error: async wait on socket failed: PQsocketPoll: "
                      << sys_error
                      << "\n";
            throw std::runtime_error(sys_error);
        }

        PQconsumeInput(conn);
        unique_notify notify;
        while ((notify = unique_notify(PQnotifies(conn))) != nullptr) {
            // todo callback
            std::cout << "NOTIFY RECEIVED: " << notify->extra
                      << " FROM " << notify->be_pid
                      << " ON CHANNEL " << notify->relname
                      << "\n";
            terminate();
            PQconsumeInput(conn);
        }
    }
}

void
lpq::db_notify_awaiter::terminate() {
    _stop.test_and_set();
}
