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
 * test/LindaDB/tree_payloads/scalar_payload --
 *   Tests for the scalar_payload class of the tree indices.
 */

#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/payload/scalar_payload.hxx>

namespace lit = ldb::index::tree;
namespace lps = lit::payloads;

// NOLINTNEXTLINE
TEST_CASE("scalar_payload conforms to payload") {
    STATIC_CHECK(lit::payload<lps::scalar_payload<int, int>>);
}

// NOLINTNEXTLINE
TEST_CASE("scalar_payload default initializes as empty") {
    const lps::scalar_payload<int, int> sut;

    CHECK(sut.empty());
}

// NOLINTNEXTLINE
TEST_CASE("scalar_payload default initializes as not full") {
    const lps::scalar_payload<int, int> sut;

    CHECK_FALSE(sut.full());
}

// NOLINTNEXTLINE
TEST_CASE("scalar_payload default initializes as having zero size") {
    const lps::scalar_payload<int, int> sut;

    CHECK(sut.size() == 0); // NOLINT(*-container-size-empty)
}

// NOLINTNEXTLINE
TEST_CASE("scalar_payload default initializes as having capacity one") {
    const lps::scalar_payload<int, int> sut;

    CHECK(sut.capacity() == 1);
}

// NOLINTNEXTLINE
TEST_CASE("empty scalar_payload does not have priority") {
    const lps::scalar_payload<int, int> sut;

    CHECK_FALSE(sut.have_priority());
}

constexpr const static auto Test_Key = 42;
constexpr const static auto Test_Value = 42;

// NOLINTNEXTLINE
TEST_CASE("empty scalar_payload can add new key-value") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    CHECK(succ);
}

// NOLINTNEXTLINE
TEST_CASE("empty scalar_payload can squish-add new key-value") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.force_set(Test_Key, Test_Value);
    CHECK(succ == std::nullopt);
}

// NOLINTNEXTLINE
TEST_CASE("empty scalar_payload becomes full after adding key-value") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    CHECK(sut.full());
}

// NOLINTNEXTLINE
TEST_CASE("empty scalar_payload becomes full after squish-adding key-value") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.force_set(Test_Key, Test_Value);
    REQUIRE(succ == std::nullopt);
    CHECK(sut.full());
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload cannot add new key-value") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key + 1, Test_Value);
    CHECK_FALSE(succ);
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload returns old key-value for squish-add new key-value") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    auto old = sut.force_set(Test_Key + 1, Test_Value);
    REQUIRE(old);
    CHECK(old->first == Test_Key); // NOLINT(*-unchecked-optional-access)
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload reports size as one") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    CHECK(sut.size() == 1);
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload updates the value for the same key") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    succ = sut.try_set(Test_Key, Test_Value);
    CHECK(succ);
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload updates the value for the same key with squish-add") {
    lps::scalar_payload<int, int> sut;

    auto succ = sut.try_set(Test_Key, Test_Value);
    REQUIRE(succ);
    auto old = sut.force_set(Test_Key, Test_Value);
    CHECK_FALSE(old);
}

// NOLINTNEXTLINE
TEST_CASE("kv-initialized scalar_payload is full") {
    const lps::scalar_payload<int, int> sut(Test_Key, Test_Value);

    CHECK(sut.full());
}

// NOLINTNEXTLINE
TEST_CASE("empty scalar_payload always compares equal") {
    const lps::scalar_payload<int, int> sut;

    CHECK(sut == 0);
    CHECK(0 == sut);
    CHECK_FALSE(sut != 0);
    CHECK_FALSE(0 != sut);
    CHECK_FALSE(sut < 0);
    CHECK_FALSE(sut > 0);
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload compares equivalently to its key") {
    const lps::scalar_payload<int, int> sut(Test_Key, Test_Value);

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
TEST_CASE("empty scalar_payload as string contains 1 and 0 for capacity/size") {
    const lps::scalar_payload<int, int> sut;
    std::ostringstream ss;
    ss << sut;
    CHECK(ss.str().find(std::to_string(1)) != std::string::npos);
    CHECK(ss.str().find(std::to_string(0)) != std::string::npos);
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload as string contains its key and value") {
    const lps::scalar_payload<int, int> sut(Test_Key, Test_Value);
    std::ostringstream ss;
    ss << sut;
    CHECK(ss.str().find(std::to_string(Test_Key)) != std::string::npos);
    CHECK(ss.str().find(std::to_string(Test_Value)) != std::string::npos);
}

// NOLINTNEXTLINE
TEST_CASE("empty scalar_payload returns nullopt in try_get") {
    const lps::scalar_payload<int, int> sut;
    CHECK(sut.try_get(Test_Key) == std::nullopt);
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload returns nullopt in try_get with different key") {
    const lps::scalar_payload<int, int> sut(Test_Key, Test_Value);
    CHECK(sut.try_get(Test_Key + 1) == std::nullopt);
}

// NOLINTNEXTLINE
TEST_CASE("full scalar_payload returns Some(value) in try_get with correct key") {
    const lps::scalar_payload<int, int> sut(Test_Key, Test_Value);
    CHECK(sut.try_get(Test_Key) != std::nullopt);
    CHECK(sut.try_get(Test_Key) == std::optional{Test_Value});
}
