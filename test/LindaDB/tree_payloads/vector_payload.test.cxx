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
 * Originally created: 2023-10-14.
 *
 * test/LindaDB/tree_payloads/vector_payload --
 *   Tests for the vector_payload class of the tree indices.
 */

#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/payload/vector_payload.hxx>

namespace lit = ldb::index::tree;
namespace lps = lit::payloads;
constexpr const static auto sut_size = 2;
template<std::size_t s = sut_size>
using sut_type = lps::vector_payload<int, int, s>;

// NOLINTNEXTLINE
TEST_CASE("vector_payload conforms to payload") {
    STATIC_CHECK(lit::payload<sut_type<>>);
}

// NOLINTNEXTLINE
TEST_CASE("vector_payload default initializes as empty") {
    const sut_type<> sut;

    CHECK(sut.empty());
}

// NOLINTNEXTLINE
TEST_CASE("vector_payload default initializes as not full") {
    const sut_type<> sut;

    CHECK_FALSE(sut.full());
}

// NOLINTNEXTLINE
TEST_CASE("vector_payload default initializes as having zero size") {
    const sut_type<> sut;

    CHECK(sut.size() == 0); // NOLINT(*-container-size-empty)
}

// NOLINTNEXTLINE
TEST_CASE("vector_payload default initializes as having capacity of nonzero") {
    const sut_type<> sut;

    CHECK(sut.capacity() == sut_size);
}

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload has priority") {
    const sut_type<> sut;

    CHECK(sut.have_priority());
}

// ordering of test keys: Test_Key3 < Test_Key < Test_Key2
// the test suite relies on this ordering, so even if
// modifying the keys retain this property
constexpr const static auto Test_Key = 42;
constexpr const static auto Test_Key2 = 420;
constexpr const static auto Test_Key3 = 7;
constexpr const static auto Test_Value = 42;

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload can add new key-value") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    CHECK(succ);
}

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload can squish-add new key-value") {
    sut_type<> sut;

    auto succ = sut.force_set(Test_Key, Test_Value);
    CHECK(succ == std::nullopt);
}

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload becomes full after adding multiple key-values") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key2, Test_Value);
    REQUIRE(succ);
    CHECK(sut.full());
}

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload becomes full after squish-adding multiple key-values") {
    sut_type<> sut;

    auto succ = sut.force_set(Test_Key, Test_Value);
    REQUIRE(succ == std::nullopt);
    succ = sut.force_set(Test_Key2, Test_Value);
    REQUIRE(succ == std::nullopt);
    CHECK(sut.full());
}

// NOLINTNEXTLINE
TEST_CASE("full vector_payload cannot add new key-value") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key2, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key3, Test_Value);
    CHECK_FALSE(succ);
}

// NOLINTNEXTLINE
TEST_CASE("full vector_payload returns old key-value for squish-add new key-value") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key2, Test_Value);
    REQUIRE(succ);
    auto old = sut.force_set(Test_Key3, Test_Value);
    REQUIRE(old);
    CHECK((old->first == Test_Key || old->first == Test_Key2)); // NOLINT(*-unchecked-optional-access)
}

// NOLINTNEXTLINE
TEST_CASE("vector_payload with one element reports size as one") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    CHECK(sut.size() == 1);
}

// NOLINTNEXTLINE
TEST_CASE("vector_payload with one element updates its value upon insertion") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key, Test_Value + 1);
    CHECK(succ);
    CHECK(sut.size() == 1);
}

// NOLINTNEXTLINE
TEST_CASE("full vector_payload updates the value for the same key") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key2, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key, Test_Value);
    CHECK(succ);
}

// NOLINTNEXTLINE
TEST_CASE("full vector_payload updates the value for the same key with squish-add") {
    sut_type<> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key2, Test_Value);
    REQUIRE(succ);
    auto old = sut.force_set(Test_Key, Test_Value);
    CHECK_FALSE(old);
}

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload always compares equal") {
    const sut_type<> sut;

    CHECK(sut == 0);
    CHECK(0 == sut);
    CHECK_FALSE(sut != 0);
    CHECK_FALSE(0 != sut);
    CHECK_FALSE(sut < 0);
    CHECK_FALSE(sut > 0);
}

// NOLINTNEXTLINE
TEST_CASE("single element vector_payload compares equivalently to its key") {
    const sut_type<> sut(Test_Key, Test_Value);

    CHECK(sut == Test_Key);
    CHECK(Test_Key == sut);
    CHECK_FALSE(sut != Test_Key);
    CHECK_FALSE(Test_Key != sut);
    CHECK_FALSE(sut < Test_Key);
    CHECK_FALSE(sut > Test_Key);
    CHECK(sut > Test_Key - 1);
    CHECK(sut < Test_Key + 1);
}

// NOLINTNEXTLINE
TEST_CASE("multi element vector_payload compares equivalently to its min/max keys") {
    // ordering of test keys: Test_Key3 < Test_Key < Test_Key2
    sut_type<> sut(Test_Key3, Test_Value);
    std::ignore = sut.force_set(Test_Key2, Test_Value);

    CHECK(sut == Test_Key);
    CHECK(Test_Key == sut);
    CHECK_FALSE(sut != Test_Key);
    CHECK_FALSE(Test_Key != sut);
    CHECK_FALSE(sut < Test_Key);
    CHECK_FALSE(sut > Test_Key);
    CHECK_FALSE(sut > Test_Key);
    CHECK_FALSE(sut < Test_Key);
    CHECK(sut > Test_Key3 - 1);
    CHECK(sut < Test_Key2 + 1);
}

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload as string contains capacity and 0 for size") {
    const sut_type<> sut;
    std::ostringstream ss;
    ss << sut;
    CHECK(ss.str().find(std::to_string(sut_size)) != std::string::npos);
    CHECK(ss.str().find(std::to_string(0)) != std::string::npos);
}

// NOLINTNEXTLINE
TEST_CASE("full vector_payload as string contains its key and value") {
    const sut_type<> sut(Test_Key, Test_Value);
    std::ostringstream ss;
    ss << sut;
    CHECK(ss.str().find(std::to_string(Test_Key)) != std::string::npos);
    CHECK(ss.str().find(std::to_string(Test_Value)) != std::string::npos);
}

// NOLINTNEXTLINE
TEST_CASE("empty vector_payload returns nullopt in try_get") {
    const sut_type<> sut;
    CHECK(sut.try_get(Test_Key) == std::nullopt);
}

// NOLINTNEXTLINE
TEST_CASE("full vector_payload returns nullopt in try_get with different key") {
    const sut_type<> sut(Test_Key, Test_Value);
    CHECK(sut.try_get(Test_Key + 1) == std::nullopt);
}

// NOLINTNEXTLINE
TEST_CASE("full vector_payload returns Some(value) in try_get with correct key") {
    const sut_type<> sut(Test_Key, Test_Value);
    CHECK(sut.try_get(Test_Key) != std::nullopt);
    CHECK(sut.try_get(Test_Key) == std::optional{Test_Value});
}

TEST_CASE("multi-element vector_payload remains sorted after insert") {
    // ordering of test keys: Test_Key3 < Test_Key < Test_Key2
    sut_type<3> sut(Test_Key3, Test_Value);
    std::ignore = sut.try_set(Test_Key2, Test_Value);
    std::ignore = sut.try_set(Test_Key, Test_Value);
    CHECK(sut == Test_Key);
    CHECK(sut > Test_Key3 - 1);
    CHECK(sut < Test_Key2 + 1);
}
