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
 * Originally created: 2023-10-20.
 *
 * test/LindaDB/lv/linda_tuple --
 *   Tests for the lv::linda_tuple type.
 */


#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/linda_tuple.hxx>

namespace lv = ldb::lv;

// NOLINTNEXTLINE
TEST_CASE("linda_tuples compare equal if they share the same elements") {
    SECTION("0-tuple") {
        const lv::linda_tuple t1;
        const lv::linda_tuple t2;
        CHECK(t1 == t2);
    }

    SECTION("1-tuple") {
        const lv::linda_tuple t1(2);
        const lv::linda_tuple t2(2);
        CHECK(t1 == t2);
    }

    SECTION("2-tuple") {
        const lv::linda_tuple t1(2, "3");
        const lv::linda_tuple t2(2, "3");
        CHECK(t1 == t2);
    }

    SECTION("3-tuple") {
        const lv::linda_tuple t1(2, "3", 4.0);
        const lv::linda_tuple t2(2, "3", 4.0);
        CHECK(t1 == t2);
    }

    SECTION("4-tuple") {
        const lv::linda_tuple t1(2, "3", 4.0, 5ULL);
        const lv::linda_tuple t2(2, "3", 4.0, 5ULL);
        CHECK(t1 == t2);
    }

    SECTION("5-tuple") {
        const lv::linda_tuple t1(2, "3", 4.0, 5ULL, 6);
        const lv::linda_tuple t2(2, "3", 4.0, 5ULL, 6);
        CHECK(t1 == t2);
    }
}

// NOLINTNEXTLINE
TEST_CASE("linda_tuples compare less based on size") {
    SECTION("0-tuple") {
        const lv::linda_tuple t1;
        const lv::linda_tuple t2(2);
        CHECK(t1 < t2);
        CHECK_FALSE(t1 > t2);
    }

    SECTION("1-tuple") {
        const lv::linda_tuple t1(3);
        const lv::linda_tuple t2(2, 2);
        CHECK(t1 < t2);
        CHECK_FALSE(t1 > t2);
    }

    SECTION("2-tuple") {
        const lv::linda_tuple t1(3, "4");
        const lv::linda_tuple t2(2, "3", 4.0);
        CHECK(t1 < t2);
        CHECK_FALSE(t1 > t2);
    }

    SECTION("3-tuple") {
        const lv::linda_tuple t1(3, "4", 5.0);
        const lv::linda_tuple t2(2, "3", 4.0, 5ULL);
        CHECK(t1 < t2);
        CHECK_FALSE(t1 > t2);
    }

    SECTION("4-tuple") {
        const lv::linda_tuple t1(3, "4", 5.0, 6ULL);
        const lv::linda_tuple t2(2, "3", 4.0, 5ULL, 6);
        CHECK(t1 < t2);
        CHECK_FALSE(t1 > t2);
    }

    SECTION("5-tuple") {
        const lv::linda_tuple t1(3, "4", 5.0, 6ULL, 7);
        const lv::linda_tuple t2(2, "3", 4.0, 5ULL, 6, 7);
        CHECK(t1 < t2);
        CHECK_FALSE(t1 > t2);
    }
}

// NOLINTNEXTLINE
TEST_CASE("linda_tuples report their size") {
    SECTION("0-tuple") {
        const lv::linda_tuple t1;
        CHECK(t1.size() == 0);
    }

    SECTION("1-tuple") {
        const lv::linda_tuple t1(3);
        CHECK(t1.size() == 1);
    }

    SECTION("2-tuple") {
        const lv::linda_tuple t1(3, "4");
        CHECK(t1.size() == 2);
    }

    SECTION("3-tuple") {
        const lv::linda_tuple t1(3, "4", 5.0);
        CHECK(t1.size() == 3);
    }

    SECTION("4-tuple") {
        const lv::linda_tuple t1(3, "4", 5.0, 6ULL);
        CHECK(t1.size() == 4);
    }

    SECTION("5-tuple") {
        const lv::linda_tuple t1(3, "4", 5.0, 6ULL, 7);
        CHECK(t1.size() == 5);
    }
}

TEST_CASE("linda_tuples can be indexed-into") {
    lv::linda_tuple t3(3, "4", 5.0);
    lv::linda_tuple t4(3, "4", 5.0, 6ULL);
    lv::linda_tuple t5(3, "4", 5.0, 6ULL, 7);

    SECTION("3-tuple") {
        CHECK(t3[0] == lv::linda_value(3));
        CHECK(t4[2] == lv::linda_value(5.0));
    }

    SECTION("4-tuple") {
        CHECK(t3[0] == lv::linda_value(3));
        CHECK(t4[2] == lv::linda_value(5.0));
        CHECK(t4[3] == lv::linda_value(6ULL));
    }

    SECTION("5-tuple") {
        CHECK(t3[0] == lv::linda_value(3));
        CHECK(t5[3] == lv::linda_value(6ULL));
        CHECK(t5[4] == lv::linda_value(7));
    }
}

TEST_CASE("const linda_tuples can be indexed-into") {
    const lv::linda_tuple t3(3, "4", 5.0);
    const lv::linda_tuple t4(3, "4", 5.0, 6ULL);
    const lv::linda_tuple t5(3, "4", 5.0, 6ULL, 7);

    SECTION("3-tuple") {
        CHECK(t3[0] == lv::linda_value(3));
        CHECK(t4[2] == lv::linda_value(5.0));
    }

    SECTION("4-tuple") {
        CHECK(t3[0] == lv::linda_value(3));
        CHECK(t4[2] == lv::linda_value(5.0));
        CHECK(t4[3] == lv::linda_value(6ULL));
    }

    SECTION("5-tuple") {
        CHECK(t3[0] == lv::linda_value(3));
        CHECK(t5[3] == lv::linda_value(6ULL));
        CHECK(t5[4] == lv::linda_value(7));
    }
}
