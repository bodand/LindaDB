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
 * src/LindaPq/lindapq/include/pg_conn_pool --
 *   
 */
#ifndef PG_CONN_POOL_HXX
#define PG_CONN_POOL_HXX

#include <forward_list>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <lpq/db_context.hxx>
#include <ldb/common.hxx>
#include <ldb/profile.hxx>

namespace lpq {
    struct wrapped_db_context;

    struct pg_conn_pool {
        constexpr const static auto cfg_max_connections = 32;
        constexpr const static auto cfg_initial_connections = cfg_max_connections / 4;

        pg_conn_pool();

        wrapped_db_context
        receive();

        void
        release(db_context* db);

    private:
        friend wrapped_db_context;

        LDBT_MUTEX(_wait_list_mtx);
        LDBT_CV(_wait_list_cv);
        std::queue<db_context*> _wait_list{};
        LDBT_MUTEX(_contexts_mtx);
        std::forward_list<db_context> _contexts{};
        std::atomic<std::size_t> _contexts_sz{cfg_initial_connections};
    };

    struct wrapped_db_context {
        db_context*
        operator->() const noexcept { return _ref; }

        db_context&
        operator*() const noexcept { return *_ref; }

        ~wrapped_db_context() noexcept { _owner.release(_ref); }

    private:
        friend pg_conn_pool;

        wrapped_db_context(db_context* ref, pg_conn_pool& owner)
            : _ref(ref),
              _owner(owner) {
        }

        db_context* _ref;
        pg_conn_pool& _owner;
    };
}

#endif
