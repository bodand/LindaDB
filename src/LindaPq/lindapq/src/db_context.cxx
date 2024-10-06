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
 * src/LindaPq/lindapq/src/db_context --
 *   
 */

#include <charconv>
#include <cmath>
#include <format>
#include <iostream>
#include <optional>
#include <string>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/meta.hxx>
#include <lpq/db_context.hxx>
#include <lpq/db_notify_awaiter.hxx>

#include <libpq-fe.h>

lpq::db_notify_awaiter
lpq::db_context::listen(const std::string_view channel) {
    return db_notify_awaiter(*this, channel);
}

std::string
lpq::db_context::next_name() {
    const auto name_int = _stmt_namer.fetch_add(1) + 1;
    std::string name(static_cast<std::size_t>(std::floor(std::log10(name_int))) + 2, '_');
    std::to_chars(name.data() + 1, name.data() + name.size(), name_int);
    return name;
}

void
lpq::db_context::conn_freer::operator()(void* conn) const noexcept {
    const auto real_conn = static_cast<PGconn*>(conn);
    PQfinish(real_conn);
}

namespace {
    const char*
    env_or_default(const char* env, const char* def) {
        const char* val = getenv(env);
        if (!val) return def;
        return val;
    }

    struct result_freer {
        void
        operator()(PGresult* res) const noexcept {
            PQclear(res);
        }
    };
    using unique_result = std::unique_ptr<PGresult, result_freer>;

    constexpr const char* const keys[] = {
           "host",
           "port",
           "user",
           "password",
           "dbname",
           nullptr};
}

lpq::db_context::
db_context() {
    const char* const values[] = {
           env_or_default("LDB_PG_HOST", "127.0.0.1"),
           env_or_default("LDB_PG_PORT", "5432"),
           env_or_default("LDB_PG_USER", "postgres"),
           env_or_default("LDB_PG_PASS", "postgres"),
           env_or_default("LDB_PG_DB", "postgres"),
           nullptr};

    _conn = conn_type(PQconnectdbParams(keys, values, false));
    if (const auto conn = static_cast<PGconn*>(_conn.get());
        !conn || PQstatus(conn) != CONNECTION_OK) {
        const auto postgres_error = PQerrorMessage(conn);
        std::cerr << "fatal: LindaDB\\lindapq cannot establish connection to postgres server: "
                  << postgres_error << ": params="
                  << "{Server=" << values[0] << ":" << values[1] << "}"
                  << "{User=" << values[2] << "}"
                  << "{Password=" << values[3] << "}"
                  << "{Database=" << values[4] << "}"
                  << "\n";
        throw std::runtime_error(postgres_error);
    }
}

std::string
lpq::db_context::prepare(std::string_view sql) {
    const auto conn = static_cast<PGconn*>(_conn.get());
    auto name = next_name();

    unique_result res(PQprepare(conn, name.c_str(), sql.data(), 0, nullptr));
    if (!res || PQresultStatus(res.get()) != PGRES_COMMAND_OK) {
        const auto postgres_error = PQerrorMessage(conn);
        std::cerr << "error: cannot prepare statement: " << PQerrorMessage(conn) << ": "
                  << sql << "\n";
        throw std::runtime_error(postgres_error);
    }
    return name;
}

namespace {
    template<class Int>
    ldb::lv::linda_value
    parse_numeric(std::string_view data) {
        Int ret{};
        std::from_chars(data.data(), data.data() + data.size(), ret);
        return ldb::lv::make_linda_value(ret);
    }

    ldb::lv::linda_value
    parse_field(const std::string_view str) {
        std::uint8_t data_type;
        const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), data_type, 16);
        if (ec != std::errc{} || *ptr != '@')
            data_type = ldb::meta::index_of_type<std::string, ldb::lv::linda_value>;

        const auto data_offset = static_cast<std::size_t>(ptr - str.data() + 1);

        switch (data_type) {
        case ldb::meta::index_of_type<std::int16_t, ldb::lv::linda_value>:
            return parse_numeric<std::int16_t>(str.substr(data_offset));

        case ldb::meta::index_of_type<std::uint16_t, ldb::lv::linda_value>:
            return parse_numeric<std::uint16_t>(str.substr(data_offset));

        case ldb::meta::index_of_type<std::int32_t, ldb::lv::linda_value>:
            return parse_numeric<std::int32_t>(str.substr(data_offset));

        case ldb::meta::index_of_type<std::uint32_t, ldb::lv::linda_value>:
            return parse_numeric<std::uint32_t>(str.substr(data_offset));

        case ldb::meta::index_of_type<std::int64_t, ldb::lv::linda_value>:
            return parse_numeric<std::int64_t>(str.substr(data_offset));

        case ldb::meta::index_of_type<std::uint64_t, ldb::lv::linda_value>:
            return parse_numeric<std::uint64_t>(str.substr(data_offset));

        case ldb::meta::index_of_type<std::string, ldb::lv::linda_value>:
            return ldb::lv::make_linda_value(str.substr(data_offset));

        case ldb::meta::index_of_type<float, ldb::lv::linda_value>:
            return parse_numeric<float>(str.substr(data_offset));

        case ldb::meta::index_of_type<double, ldb::lv::linda_value>:
            return parse_numeric<double>(str.substr(data_offset));

        case ldb::meta::index_of_type<ldb::lv::fn_call_tag, ldb::lv::linda_value>:
            return ldb::lv::make_linda_value(ldb::lv::fn_call_tag());

        case ldb::meta::index_of_type<ldb::lv::ref_type, ldb::lv::linda_value>:
            return parse_numeric<std::int8_t>(str.substr(data_offset));

        case ldb::meta::index_of_type<ldb::lv::fn_call_holder, ldb::lv::linda_value>:
            assert_that(false, "DB cannot return fn call holder object");

        default:
            LDB_UNREACHABLE;
        }
    }

}

std::optional<ldb::lv::linda_tuple>
lpq::db_context::exec_prepared(const std::string& stmt, const std::span<const char*> params) const {
    const auto conn = static_cast<PGconn*>(_conn.get());

    const auto res = unique_result(
           PQexecPrepared(conn,
                          stmt.c_str(),
                          params.size(),
                          params.data(),
                          nullptr,
                          nullptr,
                          0));
    const auto cmd_ok = PQresultStatus(res.get()) == PGRES_TUPLES_OK
                        || PQresultStatus(res.get()) == PGRES_COMMAND_OK;
    const auto row_count = PQntuples(res.get());

    if (!res
        || !cmd_ok
        || row_count > 1) {
        std::cerr << "error: stmt failed: " << PQerrorMessage(conn) << "\n";
        return {};
    }

    if (row_count == 0) return {};

    const int fields_sz = PQnfields(res.get());
    std::vector<ldb::lv::linda_value> _vals(static_cast<unsigned>(fields_sz));

    for (int i = 0; i < fields_sz; ++i) {
        const auto field = PQgetvalue(res.get(), 0, i);
        if (field) _vals[static_cast<unsigned>(i)] = parse_field(field);
    }

    return ldb::lv::linda_tuple(_vals);
}

void
lpq::db_context::deallocate(const std::string& stmt) const {
    const auto conn = static_cast<PGconn*>(_conn.get());
    PQclear(PQexec(conn, std::format("DEALLOCATE {}", stmt).c_str()));
}
