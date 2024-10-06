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
 * src/LindaPq/lindapq/include/query_tuple_to_sql --
 *   Interface for translating a given internal query_tuple to a SQL query to
 *   be run against the Postgres db.
 */

#ifndef QUERY_TUPLE_TO_SQL_HXX
#define QUERY_TUPLE_TO_SQL_HXX

#include <algorithm>
#include <array>
#include <format>
#include <numeric>
#include <string_view>

#include <ldb/query.hxx>
#include <lpq/db_context.hxx>

namespace lpq {
    struct param_types {
        std::size_t total_params;
        std::size_t scalar_params;
        std::size_t array_params;
    };

    namespace cfg {
        constexpr const static auto relation_name = std::string_view("linda_data");
        constexpr const static auto direct_fields = std::array<std::string_view, 3>{"first", "second", "third"};
        constexpr const static auto overflow_field = std::string_view("others");
    }

    inline auto
    translate_insert(const ldb::lv::linda_tuple& tuple) {
        constexpr const static auto insert_sql_shell = std::string_view("CALL insert_linda($1::LV[])");
        const auto param_data = param_types{tuple.size(), 0, tuple.size()};
        return std::make_pair(insert_sql_shell, param_data);
    }

    template<ldb::tuple_queryable Query>
    struct query_to_sql_mapper {
        explicit
        query_to_sql_mapper(db_context&, Query query)
             : _query_tuple(query.as_representing_tuple()) { }

        auto
        translate_search() {
            const auto [cond, n] = generate_condition();
            const auto sql = std::format(sql_read_shell,
                                         generate_select_fields(),
                                         cfg::relation_name,
                                         cond);
            return std::make_pair(sql, param_types{n, n, 0});
        }

        auto
        translate_remove() {
            const auto [cond, n] = generate_condition();
            const auto scalar_params = std::min(n, cfg::direct_fields.size());
            const auto sql = std::format(sql_in_shell,
                                         generate_select_fields("rel."),
                                         cfg::relation_name,
                                         cond);
            return std::make_pair(sql, param_types{n, n, 0});
        }

    private:
        constexpr const static auto sql_read_shell = std::string_view(R"(SELECT {0} FROM {1} WHERE {2} LIMIT 1;)");
        constexpr const static auto sql_in_shell = std::string_view(R"(WITH finder(ctid) AS (SELECT ctid FROM {1} WHERE {2} FOR UPDATE LIMIT 1)
DELETE FROM {1} rel USING finder WHERE rel.ctid = finder.ctid RETURNING {0};)");

        std::pair<std::string, std::size_t>
        generate_condition() {
            std::string acc;
            std::size_t n{1};
            sql_visitor visitor{n};
            for (std::size_t i = 0; i < _query_tuple.size() + 1; ++i) {
                if (!acc.empty()) acc += " AND ";
                if (i < _query_tuple.size()) {
                    acc += std::visit(visitor, _query_tuple[i]);
                }
                else {
                    acc += std::format("{} IS NULL", nth_field(i + 1));
                }
            }
            return std::make_pair(acc, _query_tuple.size());
        }

        struct sql_visitor {
            auto
            operator()(ldb::lv::ref_type) const {
                const auto current_n = n++;
                return std::format("{} = ${}::LV_TYPE", nth_field(current_n), current_n);
            }

            auto
            operator()(const auto&) const {
                const auto current_n = n++;
                return std::format("{} = ${}::LV", nth_field(current_n), current_n);
            }

            std::size_t& n;
        };

        std::string
        generate_select_fields(std::string_view prefix = "") const {
            std::string acc;
            // warning: unconventional for-loop
            for (std::size_t i = 1; i <= _query_tuple.size(); ++i) {
                if (!acc.empty()) acc += ", ";
                acc += prefix;
                acc += nth_field(i);
            }
            return acc;
        }

        static std::string
        nth_field(const std::size_t n) {
            if (n - 1 < cfg::direct_fields.size()) return std::string(cfg::direct_fields[n - 1]);
            return std::format("{}[{}]", cfg::overflow_field, n - cfg::direct_fields.size());
        }

        ldb::lv::linda_tuple _query_tuple;
    };
}

#endif
