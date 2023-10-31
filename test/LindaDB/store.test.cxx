/* LindaDB project
 *
 * Copyright (c) 2023 András Bodor <bodand@pm.me>
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
 * Originally created: 2023-10-29.
 *
 * test/LindaDB/store --
 *   Tests for the main store object representing a full IMDB instance.
 */


#include <concepts>

#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query_tuple.hxx>
#include <ldb/store.hxx>

namespace lv = ldb::lv;

TEST_CASE("store is default constructible") {
    STATIC_CHECK(std::constructible_from<ldb::store>);
}

TEST_CASE("store can store and rdp by value a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);
    auto ret = store.rdp(ldb::query_tuple("asd", 2));
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and rdp by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.rdp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           1));
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store for rdp of non-existent tuple by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.rdp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           0));
    REQUIRE_FALSE(ret.has_value());
}

TEST_CASE("store can store and rdp by type a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    int val = 0;
    auto ret = store.rdp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(lv::linda_value(val) == tuple[1]);
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and rdp cannot retrieve tuple with mismatched type") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    std::int64_t val = 0LL;
    auto ret = store.rdp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(val == 0LL);
    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store can store and rdp cannot retrieve tuple with mismatched value") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    auto ret = store.rdp(ldb::query_tuple("asd", 3));

    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store can repeat rdp calls for existing tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    for (int i = 0; i < 15; ++i) {
        int val = 0;
        auto ret = store.rdp(ldb::query_tuple("asd", ldb::ref(&val)));

        CHECK(lv::linda_value(val) == tuple[1]);
        REQUIRE(ret.has_value());
        CHECK(*ret == tuple);
    }
}

TEST_CASE("store can repeat rdp calls for missing tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    for (int i = 0; i < 15; ++i) {
        auto ret = store.rdp(ldb::query_tuple("asd", 3));
        CHECK_FALSE(ret.has_value());
    }
}

TEST_CASE("store can store and inp by value a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);
    auto ret = store.inp(ldb::query_tuple("asd", 2));
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and inp by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.inp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           1));
    REQUIRE(ret.has_value());
}

TEST_CASE("store for inp of non-existent tuple by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.inp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           0));
    REQUIRE_FALSE(ret.has_value());
}

TEST_CASE("store can store and inp by type a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    int val = 0;
    auto ret = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(lv::linda_value(val) == tuple[1]);
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and inp cannot retrieve tuple with mismatched type") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    std::int64_t val = 0LL;
    auto ret = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(val == 0LL);
    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store can store and inp cannot retrieve tuple with mismatched value") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    auto ret = store.inp(ldb::query_tuple("asd", 3));

    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store cannot repeat inp calls for existing tuple: only first succeeds") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    int val = 0;
    auto ret = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));
    CHECK(lv::linda_value(val) == tuple[1]);
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
    auto ret2 = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));
    CHECK_FALSE(ret2.has_value());
}

TEST_CASE("store can repeat inp calls for missing tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    for (int i = 0; i < 15; ++i) {
        auto ret = store.inp(ldb::query_tuple("asd", 3));
        CHECK_FALSE(ret.has_value());
    }
}
