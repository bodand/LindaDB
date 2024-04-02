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
 * Originally created: 2024-02-17.
 *
 * test/LindaDB/query/concrete_tuple_query --
 *   
 */

#include <concepts>

#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/concrete_tuple_query.hxx>

using index_type = ldb::index::tree::avl2_tree<int, int>;

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query is constructible from a tuple") {
    STATIC_CHECK(std::constructible_from<ldb::concrete_tuple_query<index_type>,
                                         ldb::lv::linda_tuple>);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query is copiable") {
    STATIC_CHECK(std::copyable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple tuple(1, 2);
    ldb::concrete_tuple_query<index_type> orig(tuple);
    auto copy = orig;
    CHECK(orig == copy);
    CHECK_FALSE(orig != copy);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query returns its input tuple as representing tuple") {
    STATIC_CHECK(std::copyable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple tuple(1, 2);
    ldb::concrete_tuple_query<index_type> query(tuple);
    CHECK(query.as_representing_tuple() == tuple);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query is movable") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple tuple(1, 2);
    ldb::concrete_tuple_query<index_type> orig(tuple);
    auto copy = orig;
    auto moved = std::move(orig);
    CHECK(moved == copy);
    CHECK_FALSE(moved != copy);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as greater to smaller tuple") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(cmp_tuple < query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as less to larger tuple") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 2, 3);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(cmp_tuple > query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as equal to equal tuple") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 2);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(cmp_tuple == query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as greater to member-wise less tuple") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 1);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(cmp_tuple < query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as less to member-wise greater tuple") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 3);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(cmp_tuple > query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as greater to smaller tuple-ptr") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(&cmp_tuple < query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as less to larger tuple-ptr") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 2, 3);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(&cmp_tuple > query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as equal to equal tuple-ptr") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 2);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(&cmp_tuple == query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as greater to member-wise less tuple-ptr") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 1);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(&cmp_tuple < query);
}

// NOLINTNEXTLINE
TEST_CASE("concrete_tuple_query right-compares as less to member-wise greater tuple-ptr") {
    STATIC_CHECK(std::movable<ldb::concrete_tuple_query<index_type>>);

    ldb::lv::linda_tuple cmp_tuple(1, 3);
    ldb::concrete_tuple_query<index_type> query(ldb::lv::linda_tuple{1, 2});

    CHECK(&cmp_tuple > query);
}
