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
 * test/LindaDB/lv/ref_type --
 *   
 */

#include <cstring>
#include <memory>
#include <utility>
#include <variant>

#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/linda_value.hxx>
#include <ldb/lv/ref_type.hxx>

using namespace ldb;

// NOLINTNEXTLINE
TEST_CASE("ref_type is constructible from int8_t") {
    STATIC_CHECK(std::constructible_from<lv::ref_type, std::int8_t>);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type is constructible from variant index") {
    STATIC_CHECK(std::constructible_from<lv::ref_type, decltype(std::declval<lv::linda_value>().index())>);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type equals to ref_type with equal value") {
    lv::ref_type a(1);
    lv::ref_type b(1);
    CHECK(a == b);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type is less than ref_type with smaller value") {
    lv::ref_type a(1);
    lv::ref_type b(2);
    CHECK(a < b);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type is greater than ref_type with larger value") {
    lv::ref_type a(2);
    lv::ref_type b(1);
    CHECK(a > b);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type is less-eq than ref_type with equal value") {
    lv::ref_type a(1);
    lv::ref_type b(1);
    CHECK(a <= b);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type is less-eq than ref_type with smaller value") {
    lv::ref_type a(1);
    lv::ref_type b(2);
    CHECK(a <= b);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type is greater-eq than ref_type with equal value") {
    lv::ref_type a(1);
    lv::ref_type b(1);
    CHECK(a >= b);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type is greater-eq than ref_type with larger value") {
    lv::ref_type a(2);
    lv::ref_type b(1);
    CHECK(a > b);
}

// NOLINTNEXTLINE
TEST_CASE("ref_type prints itself properly") {
    std::stringstream ss;
    lv::ref_type a(2);
    ss << a;
    CHECK(ss.str() == "(type: 2)");
}

// NOLINTNEXTLINE
TEST_CASE("ref_type r-compares equal to the represented value's type") {
    auto v = lv::make_linda_value("alma");
    lv::ref_type ref_v(v.index());
    CHECK(std::is_eq(v <=> ref_v));
    CHECK(std::is_lteq(v <=> ref_v));
    CHECK(std::is_gteq(v <=> ref_v));
}

// NOLINTNEXTLINE
TEST_CASE("ref_type r-compares less to a different value's (smaller) type") {
    auto v = lv::make_linda_value("alma");
    lv::ref_type ref_v(lv::make_linda_value(1).index());
    CHECK(std::is_gt(v <=> ref_v));
    CHECK(std::is_gteq(v <=> ref_v));
}

// NOLINTNEXTLINE
TEST_CASE("ref_type r-compares greater to a different value's (larger) type") {
    auto v = lv::make_linda_value("alma");
    lv::ref_type ref_v(lv::make_linda_value(3.14).index());
    CHECK(std::is_lt(v <=> ref_v));
    CHECK(std::is_lteq(v <=> ref_v));
}
