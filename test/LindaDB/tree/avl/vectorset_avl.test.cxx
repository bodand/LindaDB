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
 * Originally created: 2024-04-04.
 *
 * test/LindaDB/tree/avl/vectorset_avl --
 *   A list of tests to check the behavior of T-trees that only store values, and
 *   not key-values.
 */

#include <algorithm>
#include <random>
#include <ranges>

#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/payload/vectorset_payload.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query.hxx>

namespace lit = ldb::index::tree;
namespace lps = lit::payloads;
using sut_type = ldb::index::tree::avl2_tree<int, int, 2, lps::vectorset_payload<int, 2>>;
using tsut_type = ldb::index::tree::avl2_tree<ldb::lv::linda_tuple,
                                              ldb::lv::linda_tuple,
                                              2,
                                              lps::vectorset_payload<ldb::lv::linda_tuple, 2>>;

// NOLINTNEXTLINE
TEST_CASE("new vectorset AVL-tree can be constructed") {
    CHECK_NOTHROW(sut_type{});
}

// NOLINTNEXTLINE
TEST_CASE("new empty vectorset AVL-tree can insert elements") {
    sut_type sut;
    CHECK_NOTHROW(sut.insert(1));
}

// NOLINTNEXTLINE
TEST_CASE("new empty vectorset AVL-tree can insert multiple elements") {
    // NOLINTNEXTLINE
    // NOLINTNEXTLINE
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(4));
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(1));
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(1));
    }
}

// NOLINTNEXTLINE
TEST_CASE("new vectorset AVL-tree can search elements") {
    // NOLINTNEXTLINE
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(4));

        auto res = sut.search(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 1);
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(1));

        auto res = sut.search(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 1);
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(1));

        auto res = sut.search(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 1);
    }
}

// NOLINTNEXTLINE
TEST_CASE("new vectorset AVL-tree can search elements for concrete value") {
    // NOLINTNEXTLINE
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(4));

        auto res = sut.search(lit::value_lookup(1, 1));
        REQUIRE(res.has_value());
        CHECK(*res == 1);
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(1));

        auto res = sut.search(lit::value_lookup(1, 1));
        REQUIRE(res.has_value());
        CHECK(*res == 1);
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(1));

        auto res = sut.search(lit::value_lookup(1, 1));
        REQUIRE(res.has_value());
        CHECK(*res == 1);
    }
}

// NOLINTNEXTLINE
TEST_CASE("new vectorset AVL-tree can search elements for tuple with concrete values") {
    using namespace ldb;
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("increasing order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(1)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(4)));

        auto res = sut.search_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE_FALSE(res.has_value());
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("decreasing order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(4)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(1)));

        auto res = sut.search_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE_FALSE(res.has_value());
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("mixed order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(4)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(1)));

        auto res = sut.search_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_concrete_query(over_index<tsut_type>, lv::linda_tuple(1)));
        REQUIRE_FALSE(res.has_value());
    }
}

// NOLINTNEXTLINE
TEST_CASE("new vectorset AVL-tree can search elements for tuple with typed query values") {
    using namespace ldb;
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("increasing order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(1)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(4)));

        auto res = sut.search_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(1));

        res = sut.remove_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("decreasing order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(4)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(1)));

        auto res = sut.search_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(3));

        res = sut.remove_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(3));

        res = sut.remove_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("mixed order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(4)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple(1)));

        auto res = sut.search_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(3));

        res = sut.remove_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple(3));

        res = sut.remove_query(make_type_aware_query(over_index<tsut_type>, lv::linda_tuple(lv::ref_type(2))));
        REQUIRE(res.has_value());
    }
}

// NOLINTNEXTLINE
TEST_CASE("new vectorset AVL-tree can search elements for tuple with pitch-wise queries") {
    using namespace ldb;
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("increasing order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 1)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 4)));

        int val;
        auto res = sut.search_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple("AAA", 1));

        res = sut.remove_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple("AAA", 1));

        res = sut.remove_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("decreasing order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 4)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 1)));

        int val;
        auto res = sut.search_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple("AAA", 3));

        res = sut.remove_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple("AAA", 3));

        res = sut.remove_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
    }
    // NOLINTNEXTLINE

    // NOLINTNEXTLINE
    SECTION("mixed order") {
        tsut_type sut;
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 2)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 4)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 3)));
        CHECK_NOTHROW(sut.insert(lv::linda_tuple("AAA", 1)));

        int val;
        auto res = sut.search_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple("AAA", 3));

        res = sut.remove_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
        CHECK(*res == lv::linda_tuple("AAA", 3));

        res = sut.remove_query(make_piecewise_query(over_index<tsut_type>, "AAA", ldb::ref(&val)));
        REQUIRE(res.has_value());
    }
}

using storage_type = ldb::index::tree::avl2_tree<ldb::lv::linda_tuple,
                                                 ldb::lv::linda_tuple,
                                                 16,
                                                 ldb::index::tree::payloads::vectorset_payload<ldb::lv::linda_tuple, 16>>;
TEST_CASE("AAAAAAAAAAA") {
    storage_type sut;
    sut.insert(ldb::lv::linda_tuple("fib0.o"));
    sut.insert(ldb::lv::linda_tuple("fib1.o"));
    sut.insert(ldb::lv::linda_tuple("fib10.o"));
    sut.insert(ldb::lv::linda_tuple("fib100.o"));
    sut.insert(ldb::lv::linda_tuple("fib11.o"));
    sut.insert(ldb::lv::linda_tuple("fib12.o"));
    sut.insert(ldb::lv::linda_tuple("fib13.o"));
    sut.insert(ldb::lv::linda_tuple("fib14.o"));
    sut.insert(ldb::lv::linda_tuple("fib15.o"));
    sut.insert(ldb::lv::linda_tuple("fib16.o"));
    sut.insert(ldb::lv::linda_tuple("fib17.o"));
    sut.insert(ldb::lv::linda_tuple("fib18.o"));
    sut.insert(ldb::lv::linda_tuple("fib19.o"));
    sut.insert(ldb::lv::linda_tuple("fib2.o"));
    sut.insert(ldb::lv::linda_tuple("fib20.o"));
    sut.insert(ldb::lv::linda_tuple("fib21.o"));
    sut.insert(ldb::lv::linda_tuple("fib22.o"));
    sut.insert(ldb::lv::linda_tuple("fib23.o"));
    sut.insert(ldb::lv::linda_tuple("fib24.o"));
    sut.insert(ldb::lv::linda_tuple("fib25.o"));
    sut.insert(ldb::lv::linda_tuple("fib26.o"));
    sut.insert(ldb::lv::linda_tuple("fib27.o"));
    sut.insert(ldb::lv::linda_tuple("fib28.o"));
    sut.insert(ldb::lv::linda_tuple("fib29.o"));
    sut.insert(ldb::lv::linda_tuple("fib3.o"));
    sut.insert(ldb::lv::linda_tuple("fib30.o"));
    sut.insert(ldb::lv::linda_tuple("fib31.o"));
    sut.insert(ldb::lv::linda_tuple("fib32.o"));
    sut.insert(ldb::lv::linda_tuple("fib33.o"));
    sut.insert(ldb::lv::linda_tuple("fib34.o"));
    sut.insert(ldb::lv::linda_tuple("fib35.o"));
    sut.insert(ldb::lv::linda_tuple("fib36.o"));
    sut.insert(ldb::lv::linda_tuple("fib37.o"));
    sut.insert(ldb::lv::linda_tuple("fib38.o"));
    sut.insert(ldb::lv::linda_tuple("fib39.o"));
    sut.insert(ldb::lv::linda_tuple("fib4.o"));
    sut.insert(ldb::lv::linda_tuple("fib40.o"));
    sut.insert(ldb::lv::linda_tuple("fib41.o"));
    sut.insert(ldb::lv::linda_tuple("fib42.o"));
    sut.insert(ldb::lv::linda_tuple("fib43.o"));
    sut.insert(ldb::lv::linda_tuple("fib44.o"));
    sut.insert(ldb::lv::linda_tuple("fib45.o"));
    sut.insert(ldb::lv::linda_tuple("fib46.o"));
    sut.insert(ldb::lv::linda_tuple("fib47.o"));
    sut.insert(ldb::lv::linda_tuple("fib48.o"));
    sut.insert(ldb::lv::linda_tuple("fib49.o"));
    sut.insert(ldb::lv::linda_tuple("fib5.o"));
    sut.insert(ldb::lv::linda_tuple("fib50.o"));
    sut.insert(ldb::lv::linda_tuple("fib51.o"));
    sut.insert(ldb::lv::linda_tuple("fib52.o"));
    sut.insert(ldb::lv::linda_tuple("fib53.o"));
    sut.insert(ldb::lv::linda_tuple("fib54.o"));
    sut.insert(ldb::lv::linda_tuple("fib55.o"));
    sut.insert(ldb::lv::linda_tuple("fib56.o"));
    sut.insert(ldb::lv::linda_tuple("fib57.o"));
    sut.insert(ldb::lv::linda_tuple("fib58.o"));
    sut.insert(ldb::lv::linda_tuple("fib59.o"));
    sut.insert(ldb::lv::linda_tuple("fib6.o"));
    sut.insert(ldb::lv::linda_tuple("fib60.o"));
    sut.insert(ldb::lv::linda_tuple("fib61.o"));
    sut.insert(ldb::lv::linda_tuple("fib62.o"));
    sut.insert(ldb::lv::linda_tuple("fib63.o"));
    sut.insert(ldb::lv::linda_tuple("fib64.o"));
    sut.insert(ldb::lv::linda_tuple("fib65.o"));
    sut.insert(ldb::lv::linda_tuple("fib66.o"));
    sut.insert(ldb::lv::linda_tuple("fib67.o"));
    sut.insert(ldb::lv::linda_tuple("fib68.o"));
    sut.insert(ldb::lv::linda_tuple("fib69.o"));
    sut.insert(ldb::lv::linda_tuple("fib7.o"));
    sut.insert(ldb::lv::linda_tuple("fib70.o"));
    sut.insert(ldb::lv::linda_tuple("fib71.o"));
    sut.insert(ldb::lv::linda_tuple("fib72.o"));
    sut.insert(ldb::lv::linda_tuple("fib73.o"));
    sut.insert(ldb::lv::linda_tuple("fib74.o"));
    sut.insert(ldb::lv::linda_tuple("fib75.o"));
    sut.insert(ldb::lv::linda_tuple("fib76.o"));
    sut.insert(ldb::lv::linda_tuple("fib77.o"));
    sut.insert(ldb::lv::linda_tuple("fib78.o"));
    sut.insert(ldb::lv::linda_tuple("fib79.o"));
    sut.insert(ldb::lv::linda_tuple("fib8.o"));
    sut.insert(ldb::lv::linda_tuple("fib80.o"));
    sut.insert(ldb::lv::linda_tuple("fib81.o"));
    sut.insert(ldb::lv::linda_tuple("fib82.o"));
    sut.insert(ldb::lv::linda_tuple("fib83.o"));
    sut.insert(ldb::lv::linda_tuple("fib84.o"));
    sut.insert(ldb::lv::linda_tuple("fib85.o"));
    sut.insert(ldb::lv::linda_tuple("fib86.o"));
    sut.insert(ldb::lv::linda_tuple("fib87.o"));
    sut.insert(ldb::lv::linda_tuple("fib88.o"));
    sut.insert(ldb::lv::linda_tuple("fib89.o"));
    sut.insert(ldb::lv::linda_tuple("fib9.o"));
    sut.insert(ldb::lv::linda_tuple("fib90.o"));
    sut.insert(ldb::lv::linda_tuple("fib91.o"));
    sut.insert(ldb::lv::linda_tuple("fib92.o"));
    sut.insert(ldb::lv::linda_tuple("fib93.o"));
    sut.insert(ldb::lv::linda_tuple("fib94.o"));
    sut.insert(ldb::lv::linda_tuple("fib95.o"));
    sut.insert(ldb::lv::linda_tuple("fib96.o"));
    sut.insert(ldb::lv::linda_tuple("fib97.o"));
    sut.insert(ldb::lv::linda_tuple("fib98.o"));
    sut.insert(ldb::lv::linda_tuple("fib99.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib0.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib1.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib10.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib100.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib11.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib12.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib13.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib14.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib15.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib16.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib17.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib18.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib19.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib2.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib20.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib21.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib22.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib23.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib24.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib25.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib26.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib27.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib28.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib29.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib3.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib30.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib31.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib32.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib33.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib34.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib35.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib36.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib37.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib38.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib39.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib4.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib40.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib41.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib42.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib43.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib44.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib45.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib46.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib47.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib48.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib49.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib5.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib50.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib51.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib52.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib53.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib54.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib55.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib56.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib57.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib58.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib59.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib6.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib60.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib61.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib62.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib63.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib64.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib65.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib66.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib67.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib68.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib69.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib7.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib70.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib71.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib72.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib73.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib74.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib75.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib76.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib77.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib78.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib79.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib8.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib80.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib81.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib82.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib83.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib88.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib89.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib95.o"));
    sut.insert(ldb::lv::linda_tuple("get-fib96.o"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib84.o", "get-fib84.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib85.o", "get-fib85.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib86.o", "get-fib86.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib87.o", "get-fib87.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib90.o", "get-fib90.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib91.o", "get-fib91.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib92.o", "get-fib92.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib93.o", "get-fib93.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib94.o", "get-fib94.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib97.o", "get-fib97.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib98.o", "get-fib98.cxx"));
    sut.insert(ldb::lv::linda_tuple("CC", "get-fib99.o", "get-fib99.cxx"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib74.exe", "get-fib74.o fib74.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib75.exe", "get-fib75.o fib75.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib83.exe", "get-fib83.o fib83.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib84.exe", "get-fib84.o fib84.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib85.exe", "get-fib85.o fib85.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib86.exe", "get-fib86.o fib86.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib87.exe", "get-fib87.o fib87.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib88.exe", "get-fib88.o fib88.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib89.exe", "get-fib89.o fib89.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib9.exe", "get-fib9.o fib9.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib90.exe", "get-fib90.o fib90.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib91.exe", "get-fib91.o fib91.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib92.exe", "get-fib92.o fib92.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib93.exe", "get-fib93.o fib93.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib94.exe", "get-fib94.o fib94.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib95.exe", "get-fib95.o fib95.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib96.exe", "get-fib96.o fib96.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib97.exe", "get-fib97.o fib97.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib98.exe", "get-fib98.o fib98.o"));
    sut.insert(ldb::lv::linda_tuple("LINK", "get-fib99.exe", "get-fib99.o fib99.o"));

    std::ofstream f("_out.log");
    sut.apply([&f](auto& node) {
      f << node << "\n";
    });

    sut.insert(ldb::lv::linda_tuple("get-fib9.o"));

    std::ofstream f2("_out2.log");
    sut.apply([&f2](auto& node) {
      f2 << node << "\n";
    });
}
