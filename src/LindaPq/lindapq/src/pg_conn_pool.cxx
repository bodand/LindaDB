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
 * src/LindaPq/lindapq/src/pg_conn_pool --
 *   
 */

#include <libpq-fe.h>
#include <lpq/pg_conn_pool.hxx>

lpq::pg_conn_pool::pg_conn_pool() {
    LDBT_LOCK(_wait_list_mtx);
    LDBT_LOCK_EX(lck2, _contexts_mtx);

    for (auto i = 0; i < cfg_initial_connections; ++i) {
        const auto db = &_contexts.emplace_front();
        _wait_list.push(db);
    }
}

lpq::wrapped_db_context
lpq::pg_conn_pool::receive() {
    // fast-track if we have a free connection
    {
        LDBT_LOCK(_wait_list_mtx);
        if (!_wait_list.empty()) {
            auto ret = wrapped_db_context(_wait_list.front(), *this);
            _wait_list.pop();
            return ret;
        }
    }

    // if we can return a fresh connection we don't even need to lock
    // _wait_list, so other threads can peacefully return and take another
    // if they so wish to
    if (_contexts_sz.load() < cfg_max_connections) {
        _contexts_sz.fetch_add(1);
        LDBT_LOCK(_contexts_mtx);
        const auto next = &_contexts.emplace_front();
        return wrapped_db_context(next, *this);
    }

    LDBT_UQ_LOCK(_wait_list_mtx);
    _wait_list_cv.wait(lck, [this] { return !_wait_list.empty(); });
    auto ret = wrapped_db_context(_wait_list.front(), *this);
    _wait_list.pop();
    return ret;
}

void lpq::pg_conn_pool::release(db_context* db) {
    {
        LDBT_LOCK(_wait_list_mtx);
        _wait_list.push(db);
    }
    // PQreset(static_cast<PGconn*>(db->native_handle()));
    _wait_list_cv.notify_one();
}
