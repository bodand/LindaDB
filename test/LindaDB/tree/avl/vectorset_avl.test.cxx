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

TEST_CASE("new vectorset AVL-tree can be constructed") {
    CHECK_NOTHROW(sut_type{});
}

TEST_CASE("new empty vectorset AVL-tree can insert elements") {
    sut_type sut;
    CHECK_NOTHROW(sut.insert(1));
}

TEST_CASE("new empty vectorset AVL-tree can insert multiple elements") {
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(4));
    }

    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(1));
    }

    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2));
        CHECK_NOTHROW(sut.insert(4));
        CHECK_NOTHROW(sut.insert(3));
        CHECK_NOTHROW(sut.insert(1));
    }
}

TEST_CASE("new vectorset AVL-tree can search elements") {
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

TEST_CASE("new vectorset AVL-tree can search elements for concrete value") {
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

TEST_CASE("new vectorset AVL-tree can search elements for tuple with concrete values") {
    using namespace ldb;

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

TEST_CASE("new vectorset AVL-tree can search elements for tuple with typed query values") {
    using namespace ldb;

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

TEST_CASE("new vectorset AVL-tree can search elements for tuple with pitch-wise queries") {
    using namespace ldb;

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
