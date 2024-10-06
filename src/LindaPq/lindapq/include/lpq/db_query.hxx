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
 * src/LindaPq/lindapq/include/db_query --
 *   
 */
#ifndef DB_QUERY_HXX
#define DB_QUERY_HXX

#include <iostream>
#include <optional>
#include <string>

#include <lpq/db_context.hxx>
#include <lpq/query_tuple_to_sql.hxx>

namespace ldb::lv {
    struct linda_tuple;
}

namespace lpq {
    struct db_query {
        db_query(db_context& db, bool query, std::string_view sql, const param_types& param_types);

        std::optional<ldb::lv::linda_tuple>
        exec(const ldb::lv::linda_tuple& params) const;

        ~db_query() noexcept;

    private:
        db_context& db;
        bool _query;
        std::string _stmt;
        param_types _param_count;
    };

    inline db_query
    make_insert(db_context& db, const ldb::lv::linda_tuple& tup) {
        const auto [sql, param_types] = translate_insert(tup);
        return {db, false, sql, param_types};
    }

    template<ldb::tuple_queryable Query>
    db_query
    make_select(db_context& db, const Query& query) {
        const auto [sql, param_types] = query_to_sql_mapper(db, query).translate_search();
        return {db, true, sql, param_types};
    }

    template<ldb::tuple_queryable Query>
    db_query
    make_delete(db_context& db, const Query& query) {
        const auto [sql, param_types] = query_to_sql_mapper(db, query).translate_remove();
        return {db, true, sql, param_types};
    }
}

#endif
