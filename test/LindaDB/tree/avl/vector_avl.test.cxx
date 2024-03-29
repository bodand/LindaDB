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


#include <algorithm>
#include <random>
#include <ranges>

#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/index/tree/payload.hxx>

namespace lit = ldb::index::tree;
namespace lps = lit::payloads;
using sut_type = ldb::index::tree::avl2_tree<int, int, 2, lps::vector_payload<int, int, 2>>;


TEST_CASE("new vector AVL-tree can be constructed") {
    CHECK_NOTHROW(sut_type{});
}

TEST_CASE("new empty vector AVL-tree can insert elements") {
    sut_type sut;
    CHECK_NOTHROW(sut.insert(1, 2));
}

TEST_CASE("new empty vector AVL-tree can insert multiple elements") {
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1, 2));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(4, 2));
    }

    SECTION("decreasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
    }

    SECTION("mixed order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(2, 2));
        CHECK_NOTHROW(sut.insert(4, 2));
        CHECK_NOTHROW(sut.insert(3, 2));
        CHECK_NOTHROW(sut.insert(1, 2));
    }
}

TEST_CASE("new vector AVL-tree can search elements") {
    SECTION("increasing order") {
        sut_type sut;
        CHECK_NOTHROW(sut.insert(1, 2));
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

        auto res = sut.search(lit::any_value_lookup(1));
        REQUIRE(res.has_value());
        CHECK(*res == 2);
    }
}
