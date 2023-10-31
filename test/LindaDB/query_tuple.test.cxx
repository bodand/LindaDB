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
 * Originally created: 2023-10-22.
 *
 * test/LindaDB/query_tuple --
 *   Test for the query_tuple interface. Matches against tuples of the same size,
 *   same types, and same values (if given). Used to process queries to the
 *   linda tuple-space.
 */


#include <concepts>

#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query_tuple.hxx>

namespace lv = ldb::lv;

TEST_CASE("meta::matcher_type<int> is match_value<int>") {
    STATIC_CHECK(std::same_as<ldb::meta::matcher_type<int>, ldb::match_value<int>>);
}

TEST_CASE("meta::matcher_type<match_type<int>> is match_type<int>") {
    STATIC_CHECK(std::same_as<ldb::meta::matcher_type<ldb::match_type<int>>, ldb::match_type<int>>);
}

TEST_CASE("meta::make_matcher<int> is match_value<int>") {
    STATIC_CHECK(std::same_as<decltype(ldb::meta::make_matcher<int>(2)),
                              ldb::match_value<int>>);
}

TEST_CASE("meta::make_matcher<match_type<int>> is match_type<int>") {
    STATIC_CHECK(std::same_as<decltype(ldb::meta::make_matcher<ldb::match_type<int>>(ldb::ref<int>(nullptr))),
                              ldb::match_type<int>>);
}

TEST_CASE("query_tuple with different size is compared correctly") {
    const lv::linda_tuple lv("str", 2);
    const ldb::query_tuple q("str");

    SECTION("not equal") {
        CHECK(lv != q);
        CHECK_FALSE(lv == q);
    }
    SECTION("tuple greater query") {
        CHECK(lv > q);
    }
}

TEST_CASE("query_tuple with greater type field is compared correctly") {
    const lv::linda_tuple lv("str", 2);
    std::int64_t val = 0LL;
    const ldb::query_tuple q("str", ldb::ref(&val));

    SECTION("not equal") {
        CHECK_FALSE(lv == q);
        CHECK(lv != q);
    }
    SECTION("tuple less query") {
        CHECK(lv < q);
    }

    CHECK(val == 0LL);
}

TEST_CASE("query_tuple with lesser type field is compared correctly") {
    const lv::linda_tuple lv("str", 2);
    short val = 0;
    const ldb::query_tuple q("str", ldb::ref(&val));

    SECTION("not equal") {
        CHECK_FALSE(lv == q);
        CHECK(lv != q);
    }
    SECTION("tuple greater query") {
        CHECK(lv > q);
    }

    CHECK(val == 0LL);
}

TEST_CASE("query_tuple with greater value is compared correctly") {
    const lv::linda_tuple lv("str", 2);
    const ldb::query_tuple q("str", 3);

    SECTION("not equal") {
        CHECK_FALSE(lv == q);
        CHECK(lv != q);
    }
    SECTION("tuple less query") {
        CHECK(lv < q);
    }
}

TEST_CASE("query_tuple with same types is matched") {
    const lv::linda_tuple lv("str", 2);
    int val = 0;
    const ldb::query_tuple q("str", ldb::ref(&val));
    CHECK(lv == q);
    CHECK(val == 2);
}

TEST_CASE("query_tuple with same values is matched") {
    const lv::linda_tuple lv("str", 2);
    const ldb::query_tuple q("str", 2);
    CHECK(lv == q);
}

TEST_CASE("query_tuple with same values with linda_values is matched") {
    const lv::linda_tuple lv("str", 2);
    const ldb::query_tuple q(lv::linda_value("str"), lv::linda_value(2));
    CHECK(lv == q);
}
