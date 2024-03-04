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

#include <sstream>

#include <catch2/catch_template_test_macros.hpp>
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

TEST_CASE("linda_tuple can be instantiated from a span of vals") {
    SECTION("empty vals") {
        auto vals = std::vector<lv::linda_value>{};
        auto expected = lv::linda_tuple();
        auto sut = lv::linda_tuple(vals);
        CHECK(sut == expected);
    }

    SECTION("small vals") {
        auto vals = std::vector<lv::linda_value>{3, "4"};
        auto expected = lv::linda_tuple(3, "4");
        auto sut = lv::linda_tuple(vals);
        CHECK(sut == expected);
    }

    // because we know 4 is a tricky number in the implementation
    SECTION("tricky-amount of vals") {
        auto vals = std::vector<lv::linda_value>{3, "4", 5.0, 6ULL};
        auto expected = lv::linda_tuple(3, "4", 5.0, 6ULL);
        auto sut = lv::linda_tuple(vals);
        CHECK(sut == expected);
    }

    SECTION("many vals") {
        auto vals = std::vector<lv::linda_value>{3, "4", 5.0, 6ULL, 7};
        auto expected = lv::linda_tuple(3, "4", 5.0, 6ULL, 7);
        auto sut = lv::linda_tuple(vals);
        CHECK(sut == expected);
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

TEMPLATE_TEST_CASE("empty linda_tuple's begin equals its end",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType empty_tuple;

    SECTION("const iterator") {
        CHECK(empty_tuple.cbegin() == empty_tuple.cend());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            CHECK(empty_tuple.begin() == empty_tuple.end());
        }
    }
}

TEMPLATE_TEST_CASE("non-empty linda_tuple's begin doesn't equal its end",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);

    SECTION("const iterator") {
        CHECK(tuple.cbegin() != tuple.cend());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            CHECK(tuple.begin() != tuple.end());
        }
    }
}

TEMPLATE_TEST_CASE("non-empty linda_tuple's begin is < its end",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);

    SECTION("const iterator") {
        CHECK(tuple.cbegin() < tuple.cend());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            CHECK(tuple.begin() < tuple.end());
        }
    }
}

TEMPLATE_TEST_CASE("non-empty linda_tuple's end is > its begin",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);

    SECTION("const iterator") {
        CHECK(tuple.cend() > tuple.cbegin());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            CHECK(tuple.end() > tuple.begin());
        }
    }
}

TEST_CASE("default iterators are equal") {
    CHECK(lv::linda_tuple::iterator{} == lv::linda_tuple::iterator{});
}

TEST_CASE("default const iterators are equal") {
    CHECK(lv::linda_tuple::iterator{} == lv::linda_tuple::iterator{});
}

TEMPLATE_TEST_CASE("linda_tuple's end is == default iterator",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);
    using cit = TestType::const_iterator;
    using mit = TestType::iterator;

    SECTION("const iterator") {
        CHECK(tuple.cend() == cit{});
        CHECK(cit{} == tuple.cend());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            CHECK(tuple.end() == mit{});
            CHECK(mit{} == tuple.end());
        }
    }
}

TEMPLATE_TEST_CASE("non-empty linda_tuple's begin is < default iterator",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);
    using cit = TestType::const_iterator;
    using mit = TestType::iterator;

    SECTION("const iterator") {
        CHECK(tuple.cbegin() < cit{});
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            CHECK(tuple.begin() < mit{});
        }
    }
}

TEMPLATE_TEST_CASE("default iterator is > non-empty linda_tuples's begin",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);
    using cit = TestType::const_iterator;
    using mit = TestType::iterator;

    SECTION("const iterator") {
        CHECK(cit{} > tuple.cbegin());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            CHECK(mit{} > tuple.begin());
        }
    }
}

TEMPLATE_TEST_CASE("tuple iterator can be dereferenced",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);

    SECTION("const iterator") {
        const auto it = tuple.cbegin();
        CHECK(*it == tuple[0]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto it = tuple.begin();
            CHECK(*it == tuple[0]);
        }
    }
}

TEMPLATE_TEST_CASE("tuple iterator can be dereferenced through ->",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1);

    SECTION("const iterator") {
        const auto it = tuple.cbegin();
        CHECK(it->index() == tuple[0].index());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto it = tuple.begin();
            CHECK(it->index() == tuple[0].index());
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via pre-increment",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2);

    SECTION("const iterator") {
        auto it = tuple.cbegin();
        const auto new_it = ++it;
        CHECK(*it == tuple[1]);
        CHECK(*new_it == tuple[1]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.begin();
            const auto new_it = ++it;
            CHECK(*it == tuple[1]);
            CHECK(*new_it == tuple[1]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via post-increment",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2);

    SECTION("const iterator") {
        auto it = tuple.cbegin();
        const auto old_it = it++;
        CHECK(*it == tuple[1]);
        CHECK(*old_it == tuple[0]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.begin();
            const auto old_it = it++;
            CHECK(*it == tuple[1]);
            CHECK(*old_it == tuple[0]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via pre-decrement",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2);

    SECTION("const iterator") {
        auto it = tuple.cend();
        const auto new_it = --it;
        CHECK(*it == tuple[1]);
        CHECK(*new_it == tuple[1]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.end();
            const auto new_it = --it;
            CHECK(*it == tuple[1]);
            CHECK(*new_it == tuple[1]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via post-decrement",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2);

    SECTION("const iterator") {
        auto it = tuple.cend();
        const auto old_it = it--;
        CHECK(*it == tuple[1]);
        CHECK(old_it == tuple.cend());
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.end();
            const auto old_it = it--;
            CHECK(*it == tuple[1]);
            CHECK(old_it == tuple.end());
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via += <positive>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cbegin();
        const auto new_it = (it += 2);
        CHECK(*it == tuple[2]);
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.begin();
            const auto new_it = (it += 2);
            CHECK(*it == tuple[2]);
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via += <negative>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cend();
        const auto new_it = (it += -2);
        CHECK(*it == tuple[2]);
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.end();
            const auto new_it = (it += -2);
            CHECK(*it == tuple[2]);
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via -= <positive>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cend();
        const auto new_it = (it -= 2);
        CHECK(*it == tuple[2]);
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.end();
            const auto new_it = (it -= 2);
            CHECK(*it == tuple[2]);
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via -= <negative>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cbegin();
        const auto new_it = (it -= -2);
        CHECK(*it == tuple[2]);
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.begin();
            const auto new_it = (it -= -2);
            CHECK(*it == tuple[2]);
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via + <positive>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cbegin();
        const auto new_it = (it + 2);
        CHECK(*it == tuple[0]);
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.begin();
            const auto new_it = (it + 2);
            CHECK(*it == tuple[0]);
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via + <negative>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cend();
        const auto new_it = (it + -2);
        CHECK(it == tuple.cend());
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.end();
            const auto new_it = (it + -2);
            CHECK(it == tuple.end());
            CHECK(*new_it == tuple[2]);
        }
    }
}
// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via <positive> + it",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cbegin();
        const auto new_it = (2 + it);
        CHECK(*it == tuple[0]);
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.begin();
            const auto new_it = (it + 2);
            CHECK(*it == tuple[0]);
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via <negative> + it",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cend();
        const auto new_it = (-2 + it);
        CHECK(it == tuple.cend());
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.end();
            const auto new_it = (it + -2);
            CHECK(it == tuple.end());
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via - <positive>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cend();
        const auto new_it = (it - 2);
        CHECK(it == tuple.cend());
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.end();
            const auto new_it = (it - 2);
            CHECK(it == tuple.end());
            CHECK(*new_it == tuple[2]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterators can be in - operation",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        const auto a = tuple.cbegin();
        const auto b = tuple.cend();

        SECTION("greater from smaller") {
            CHECK(a == b + (a - b));
        }

        SECTION("smaller from greater") {
            CHECK(b == a + (b - a));
        }
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto a = tuple.begin();
            const auto b = tuple.end();

            SECTION("greater from smaller") {
                CHECK(a == b + (a - b));
            }

            SECTION("smaller from greater") {
                CHECK(b == a + (b - a));
            }
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterators can be in - op with sentinel iterator",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        const auto a = tuple.cbegin();
        const auto b = typename TestType::const_iterator{};

        SECTION("greater from smaller") {
            // cannot check with CHECK(a == b + (a - b));
            // because the sentinel operator cannot be added to
            CHECK(a - b == -static_cast<long long>(tuple.size()));
        }

        SECTION("smaller from greater") {
            CHECK(b == a + (b - a));
        }
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto a = tuple.begin();
            const auto b = typename TestType::iterator{};

            SECTION("greater from smaller") {
                // cannot check with CHECK(a == b + (a - b));
                // because the sentinel operator cannot be added to
                CHECK(a - b == -static_cast<long long>(tuple.size()));
            }

            SECTION("smaller from greater") {
                CHECK(b == a + (b - a));
            }
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("the difference of tuple sentinel iterators is zero",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        const auto a = typename TestType::const_iterator{};
        const auto b = typename TestType::const_iterator{};

        CHECK(a - b == 0);
        CHECK(b - a == 0);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto a = typename TestType::iterator{};
            const auto b = typename TestType::iterator{};

            CHECK(a - b == 0);
            CHECK(b - a == 0);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterator can be stepped via - <negative>",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2, 3, 4);

    SECTION("const iterator") {
        auto it = tuple.cbegin();
        const auto new_it = (it - -2);
        CHECK(*it == tuple[0]);
        CHECK(*new_it == tuple[2]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto it = tuple.begin();
            const auto new_it = (it - -2);
            CHECK(*it == tuple[0]);
            CHECK(*new_it == tuple[2]);
        }
    }
}

TEMPLATE_TEST_CASE("iterator can be indexed with > 0 index",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2);

    SECTION("const iterator") {
        const auto it = tuple.cbegin();
        CHECK(it[1] == tuple[1]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto it = tuple.begin();
            CHECK(it[1] == tuple[1]);
        }
    }
}

TEMPLATE_TEST_CASE("iterator can be indexed with < 0 index",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2);

    SECTION("const iterator") {
        const auto it = std::next(tuple.cbegin());
        CHECK(it[-1] == tuple[0]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto it = std::next(tuple.begin());
            CHECK(it[-1] == tuple[0]);
        }
    }
}

TEMPLATE_TEST_CASE("iterator can be indexed with 0 index",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    TestType tuple(1, 2);

    SECTION("const iterator") {
        const auto it = std::next(tuple.cbegin());
        CHECK(it[0] == tuple[1]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            const auto it = std::next(tuple.begin());
            CHECK(it[0] == tuple[1]);
        }
    }
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE("tuple iterators can be swapped",
                   "[linda_tuple]",
                   lv::linda_tuple,
                   const lv::linda_tuple) {
    using std::swap;
    TestType tuple(1, 2);

    SECTION("const iterator") {
        auto a = tuple.cbegin();
        auto b = std::next(tuple.cbegin());
        CHECK(*a == tuple[0]);
        CHECK(*b == tuple[1]);

        a.swap(b);
        CHECK(*a == tuple[1]);
        CHECK(*b == tuple[0]);
    }

    if constexpr (!std::is_const_v<TestType>) {
        SECTION("mutable iterator") {
            auto a = tuple.begin();
            auto b = std::next(tuple.begin());
            CHECK(*a == tuple[0]);
            CHECK(*b == tuple[1]);

            a.swap(b);
            CHECK(*a == tuple[1]);
            CHECK(*b == tuple[0]);
        }
    }
}

TEST_CASE("empty tuple prints as parentheses") {
    const lv::linda_tuple empty;
    std::ostringstream oss;
    oss << empty;
    CHECK(oss.str() == "()");
}

TEST_CASE("1-tuple prints as with singular value") {
    const lv::linda_tuple scalar(42);
    const std::string int_name = typeid(int).name();
    std::ostringstream oss;
    oss << scalar;
    CHECK(oss.str() == "((42::" + int_name + "))");
}

TEST_CASE("4-tuple prints as with ,-separated value") {
    const lv::linda_tuple scalar(1, 2, 3, 4);
    const std::string int_name = typeid(int).name();
    std::ostringstream oss;
    oss << scalar;
    CHECK(oss.str() == "((1::" + int_name + "), (2::" + int_name + "), (3::" + int_name + "), (4::" + int_name + "))");
}
