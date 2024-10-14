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
 * Originally created: 2024-08-08.
 *
 * src/LindaPq/lindapq/include/pg_store --
 *   A store type backed by Postgres
 */
#ifndef PG_STORE_HXX
#define PG_STORE_HXX

#include <thread>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query.hxx>
#include <ldb/query/tuple_query.hxx>
#include <lpq/db_context.hxx>
#include <lpq/db_notify_awaiter.hxx>
#include <lpq/db_query.hxx>

#include "pg_conn_pool.hxx"

namespace lpq {
    struct pg_store {
        using value_type = ldb::lv::linda_tuple;
        using storage_type = db_context;
        using pointer_type = value_type*;
        using query_type = ldb::tuple_query<storage_type>;

        static auto
        indices() noexcept {
            return ::ldb::over_index<storage_type>;
        }

        void
        insert(const value_type& tup) const {
            const auto db = _pool.receive();
            std::ignore = make_insert(*db, tup).exec();
        }

        std::optional<value_type>
        try_read(const query_type& query) const {
            const auto db = _pool.receive();
            return return_through(query, make_select(*db, query).exec());
        }

        template<class... Args>
        std::optional<value_type>
        try_read(Args&&... args)
            requires((
                (ldb::lv::is_linda_value_v<std::remove_cvref_t<Args>>
                 || ldb::meta::is_matcher_type_v<Args>)
                && ...)) {
            return try_read(ldb::make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        value_type
        read(const query_type& query) const {
            const auto db = _pool.receive();
            const auto db_q = make_select(*db, query);
            if (const auto res = db_q.exec()) return return_through(query, *res);

            value_type ret;
            waiting_query q(ret, db_q); {
                LDBT_LOCK(_typemaps_mtx);
                _typemaps.insert(std::make_pair(query.as_type_string(), &q));
            }
            q.cv.wait(false);
            return return_through(query, std::move(ret));
        }

        template<class... Args>
        value_type
        read(Args&&... args)
            requires((
                (ldb::lv::is_linda_value_v<std::remove_cvref_t<Args>>
                 || ldb::meta::is_matcher_type_v<Args>)
                && ...)) {
            return read(ldb::make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        std::optional<ldb::lv::linda_tuple>
        try_remove(const query_type& query) const {
            const auto db = _pool.receive();
            return return_through(query, make_delete(*db, query).exec());
        }

        template<class... Args>
        std::optional<value_type>
        try_remove(Args&&... args)
            requires((
                (ldb::lv::is_linda_value_v<std::remove_cvref_t<Args>>
                 || ldb::meta::is_matcher_type_v<Args>)
                && ...)) {
            return try_remove(ldb::make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        value_type
        remove(const query_type& query) const {
            const auto db = _pool.receive();
            const auto db_q = make_delete(*db, query);
            if (const auto res = db_q.exec()) return return_through(query, *res);

            value_type ret;
            waiting_query q(ret, db_q); {
                LDBT_LOCK(_typemaps_mtx);
                _typemaps.insert(std::make_pair(query.as_type_string(), &q));
            }
            q.cv.wait(false);
            return return_through(query, std::move(ret));
        }

        template<class... Args>
        ldb::lv::linda_tuple
        remove(Args&&... args)
            requires((
                (ldb::lv::is_linda_value_v<std::remove_cvref_t<Args>>
                 || ldb::meta::is_matcher_type_v<Args>)
                && ...)) {
            return remove(ldb::make_piecewise_query(indices(), std::forward<Args>(args)...));
        }

        pg_store();

        ~pg_store() noexcept {
            _awaiter.terminate();
            _await_looper.join();
        }

    private:
        static ldb::lv::linda_tuple
        return_through(const query_type& query, ldb::lv::linda_tuple tup) {
            std::ignore = tup <=> query; // legacy (and only) API for applying a query onto a tuple
            return tup;
        }

        static std::optional<ldb::lv::linda_tuple>
        return_through(const query_type& query, std::optional<ldb::lv::linda_tuple> tup) {
            if (tup) std::ignore = *tup <=> query; // legacy (and only) API for applying a query onto a tuple
            return tup;
        }

        struct waiting_query {
            ldb::lv::linda_tuple& result;
            const db_query& query;
            std::atomic_flag cv = ATOMIC_FLAG_INIT;

            waiting_query(ldb::lv::linda_tuple& result, const db_query& query)
                : result(result),
                  query(query) {
            }

            bool
            retry() {
                const auto res = query.exec();
                if (!res) return false;

                result = *res;
                cv.test_and_set();
                cv.notify_all();
                return true;
            }
        };

        template<class... Ts>
        struct all_of : Ts... {
            using is_transparent = int;
            using Ts::operator()...;
        };

        mutable pg_conn_pool _pool;
        mutable db_context _db;
        mutable db_notify_awaiter _awaiter;
        std::thread _await_looper;
        mutable LDBT_MUTEX(_typemaps_mtx);
        mutable std::unordered_multimap<std::string,
            waiting_query*,
            all_of<std::hash<std::string>, std::hash<std::string_view>>,
            std::equal_to<>> _typemaps;
    };
}

#endif
