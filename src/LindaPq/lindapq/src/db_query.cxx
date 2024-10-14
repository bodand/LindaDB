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
 * src/LindaPq/lindapq/src/db_query --
 *   
 */

#include <lpq/db_query.hxx>

#include <libpq-fe.h>

namespace {
    template<std::random_access_iterator It, std::sentinel_for<It> End>
    std::string
    tuple_to_pg_array(It begin, End end) {
        constexpr const static std::string_view init_buf = "{";
        std::string ret(init_buf);
        ret.reserve(static_cast<std::string::size_type>(end - begin) * 32);
        ret = std::accumulate(begin, end, //
                              std::move(ret),
                              [](std::string acc, const ldb::lv::linda_value& lv) {
                                  const auto last_size = acc.size();
                                  const auto comma_offs = last_size != 1;
                                  const auto append_size = pg_str_size(lv, true);

                                  acc.resize(last_size + append_size + comma_offs);

                                  if (last_size != 1) acc[last_size] = ',';
                                  pg_serialize_to(lv,
                                                  acc.data() + last_size + comma_offs,
                                                  acc.data() + last_size + comma_offs + append_size,
                                                  true);

                                  return acc;
                              });
        ret += '}';
        return ret;
    }
}

lpq::db_query::
db_query(db_context& db,
         const bool query,
         const std::string_view sql,
         const param_types& param_types,
         const ldb::lv::linda_tuple& tup,
         const bool prepared)
    : db(db),
      _query(query),
      _stmt(prepared ? sql : db.prepare(sql)),
      _param_count(param_types),
      _tuple(tup) {
}

std::optional<ldb::lv::linda_tuple>
lpq::db_query::exec() const {
    assert_that(_param_count.total_params == _tuple.size());
    std::vector<std::string> scalars(_param_count.scalar_params);
    const std::string array = tuple_to_pg_array(
        std::next(_tuple.begin(), static_cast<long>(_param_count.scalar_params)),
        _tuple.end());
    std::transform(_tuple.begin(),
                   std::next(_tuple.begin(), static_cast<long>(_param_count.scalar_params)),
                   scalars.begin(),
                   [this](const ldb::lv::linda_value& lv) {
                       if (_query) return pg_query_serialize(lv);
                       return pg_serialize(lv);
                   });

    std::vector<const char*> param_values(_param_count.scalar_params + (_param_count.array_params > 0));
    std::ranges::transform(scalars, param_values.begin(), [](auto& str) { return str.c_str(); });
    if (_param_count.array_params > 0) param_values.back() = array.c_str();

    return db.exec_prepared(_stmt, param_values);
}

lpq::db_query::~db_query() noexcept try {
    db.deallocate(_stmt);
} catch (...) {
    // mostly irrelevant: at most std::bad_alloc is expected & that is unlikely
    // and safely ignored here at most the server process's memory can't be
    // cleared of unused prepared stmts, but postgres can probably take care of
    // that... hopefully
}
