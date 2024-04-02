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
 * Originally created: 2023-11-17.
 *
 * test/LindaDB/tree/avl/scalar_avl --
 *   Tests for scalar payload AVL tree implementation.
 */


#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <ldb/query.hxx>

namespace lit = ldb::index::tree;
namespace lps = lit::payloads;
using sut_type = ldb::index::tree::avl2_tree<int, int, 2>;

TEST_CASE("new chime AVL-tree can be constructed") {
    CHECK_NOTHROW(sut_type{});
}

TEST_CASE("new empty chime AVL-tree can insert elements") {
    sut_type sut;
    CHECK_NOTHROW(sut.insert(1, 2));
}

TEST_CASE("new empty chime AVL-tree can insert multiple elements") {
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(1, 4));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(4, 2));
    }

    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(4, 3));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
    }

    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(2, 3));
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
    }
}

TEST_CASE("new chime AVL-tree can search elements") {
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(1, 3));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(4, 2));

        auto res = sut.search(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 2);
    }

    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(1, 4));

        auto res = sut.search(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 2);
    }

    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(1, 3));

        auto res = sut.search(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 2);
    }
}

TEST_CASE("new chime AVL-tree can remove elements") {
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(1, 3));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(4, 2));

        auto res = sut.remove(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 2);
        res = sut.remove(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 3);
    }

    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(1, 3));

        auto res = sut.remove(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 2);
        res = sut.remove(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 3);
    }

    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(1, 3));

        auto res = sut.remove(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 2);
        res = sut.remove(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 3);
    }
}

TEST_CASE("new chime AVL-tree can add elements indefinitely",
          "[.long]") {
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> key(100, 999'999);
    sut_type sut;
    for (int i = 0; i < 2'000'000; ++i) {
        auto key_val = key(rng);
        sut.insert(key_val, i);
        auto f = sut.search(lit::any_value_lookup(key_val));
        REQUIRE(f.has_value());
    }
}

TEST_CASE("new chime AVL-tree can remove elements in edge cases") {
    auto init = [](sut_type& sut) {
        sut.insert(10, 0);
        sut.insert(20, 0);
        sut.insert(30, 0);
        sut.insert(40, 0);
        sut.insert(35, 0);
        sut.insert(25, 0);
        sut.insert(1, 0);
        sut.insert(2, 0);
        sut.insert(45, 0);
    };

    SECTION("try remove non-existing") {
        sut_type sut;
        init(sut);

        auto none = sut.remove(lit::any_value_lookup(999));
        CHECK_FALSE(none.has_value());
    }

    SECTION("remove leaf") {
        sut_type sut;
        init(sut);

        auto one = sut.remove(lit::any_value_lookup(1));
        auto two = sut.remove(lit::any_value_lookup(2));
        REQUIRE(one.has_value());
        REQUIRE(two.has_value());
        CHECK(*one == 0);
        CHECK(*two == 0);
    }

    SECTION("remove half-leaf") {
        sut_type sut;
        init(sut);

        auto _35 = sut.remove(lit::any_value_lookup(35));
        REQUIRE(_35.has_value());
        CHECK(*_35 == 0);
        auto _40 = sut.remove(lit::any_value_lookup(40));
        REQUIRE(_40.has_value());
        CHECK(*_40 == 0);
    }

    SECTION("remove internal node") {
        sut_type sut;
        init(sut);

        auto ten = sut.remove(lit::any_value_lookup(10));
        auto twenty = sut.remove(lit::any_value_lookup(20));
        REQUIRE(ten.has_value());
        REQUIRE(twenty.has_value());
        CHECK(*ten == 0);
        CHECK(*twenty == 0);
    }

    SECTION("remove root") {
        sut_type sut;
        init(sut);

        auto ex_root = sut.remove(lit::any_value_lookup(30));
        REQUIRE(ex_root.has_value());
        CHECK(*ex_root == 0);
    }

    SECTION("remove with merge") {
        sut_type sut;
        init(sut);
        sut.insert(1, 1);
        sut.insert(1, 2);
        sut.insert(2, 1);
        sut.insert(2, 2);

        auto n = sut.remove(lit::any_value_lookup(10));
        REQUIRE(n.has_value());
        CHECK(*n == 0);
    }
}

TEST_CASE("new chime AVL-tree removes correct element") {
    ldb::index::tree::avl2_tree<ldb::lv::linda_value, ldb::lv::linda_tuple*> sut;
    std::vector buf{
           ldb::lv::linda_tuple("asd", 1, "dsa"),
           ldb::lv::linda_tuple("asd", 2, "dsa"),
           ldb::lv::linda_tuple("asd", 3, "dsa"),
           ldb::lv::linda_tuple("asd", 4, "dsa"),
    };
    for (unsigned i = 0; i < 4; ++i) {
        sut.insert(ldb::lv::linda_value("asd"), &buf[i]);
    }

    std::string data;
    auto res = sut.remove(ldb::index::tree::value_lookup(
           ldb::lv::linda_value("asd"),
           ldb::make_piecewise_query(ldb::over_index<decltype(sut)>,
                                     "asd",
                                     3,
                                     ldb::ref(&data))));
    REQUIRE(res.has_value());
    CHECK(*res == &buf[2]);
}

TEST_CASE("new chime AVL-tree can remove elements indefinitely",
          "[.long]") {
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> key(100, 999'999);
    sut_type sut;
    for (int i = 0; i < 2'000'000; ++i) {
        auto key_val = key(rng);
        sut.insert(key_val, i);
    }

    for (int i = 0; i < 2'000'000; ++i) {
        auto key_val = key(rng);
        auto res = sut.remove(lit::any_value_lookup(key_val));
        if (res) {
            CHECK(*res >= 0);
            CHECK(*res < 2'000'000);
        }
    }
    SUCCEED();
}

TEST_CASE("chime AVL-tree can find elems through query") {
    using index_type = ldb::index::tree::avl2_tree<ldb::lv::linda_value, ldb::lv::linda_tuple*>;
    index_type sut;
    std::vector buf{
           ldb::lv::linda_tuple("m", 2),
           ldb::lv::linda_tuple("done", 0),
           ldb::lv::linda_tuple(1),
           ldb::lv::linda_tuple("xasd", 3, "dsa"),
           ldb::lv::linda_tuple("xasd", 4, "dsa"),
           ldb::lv::linda_tuple("xasd", 5, "dsa"),
           ldb::lv::linda_tuple("xasd", 6, "dsa"),
           ldb::lv::linda_tuple("xasd", 7, "dsa"),
           ldb::lv::linda_tuple("xasd", 8, "dsa"),
           ldb::lv::linda_tuple("xasd", 9, "dsa"),
           ldb::lv::linda_tuple("xasd", 10, "dsa"),
           ldb::lv::linda_tuple("xasd", 11, "dsa"),
           ldb::lv::linda_tuple("xasd", 12, "dsa"),
    };
    for (unsigned i = 0; i < 4; ++i) {
        sut.insert(buf[i][0], &buf[i]);
    }

    int data{};
    auto querym = ldb::make_piecewise_query(ldb::over_index<index_type>, "m", ldb::ref(&data));
    std::ignore = querym.remove_via_field(0, sut);

    auto query1 = ldb::make_piecewise_query(ldb::over_index<index_type>, 1);
    auto res = query1.remove_via_field(0, sut);
}
