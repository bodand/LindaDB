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
 * Originally created: 2023-10-15.
 *
 * test/LindaDB/tree/tree_node --
 *   
 */
#include <catch/fakeit.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/payload/scalar_payload.hxx>
#include <ldb/index/tree/tree_node.hxx>
#include <ldb/index/tree/tree_node_handler.hxx>

using namespace fakeit;
namespace lit = ldb::index::tree;
namespace lps = lit::payloads;
using payload_type = lps::scalar_payload<int, int>;
using sut_type = lit::tree_node<payload_type>;

// ordering of test keys: Test_Key3 < Test_Key < Test_Key2
// the test suite relies on this ordering, so even if
// modifying the keys retain this property
constexpr const static auto Test_Key = 42;
constexpr const static auto Test_Key2 = 420;
constexpr const static auto Test_Key3 = 7;
constexpr const static auto Test_Value = 42;

TEST_CASE("tree_node is constructible with its parent") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    CHECK_NOTHROW(sut_type{obj});
    VerifyNoOtherInvocations(mock);
}

TEST_CASE("default tree_node has zero balancing factor") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};
    CHECK(sut.balance_factor() == 0);
}

TEST_CASE("default tree_node has zero size") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};
    CHECK(sut.size() == 0);
}

TEST_CASE("default tree_node is empty") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};
    CHECK(sut.empty());
}

TEST_CASE("default tree_node is not full") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};
    CHECK_FALSE(sut.full());
}

TEST_CASE("empty tree_node has inclusive height of 1") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};
    CHECK(sut.total_height_inclusive() == 1);
}

TEST_CASE("empty tree_node has inclusive height of 1 after refresh") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    sut_type sut{obj};
    sut.refresh_heights();
    CHECK(sut.total_height_inclusive() == 1);
}

TEST_CASE("tree_node has same capacity as its payload") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};
    const payload_type payload;
    CHECK(sut.capacity() == payload.capacity());
}

TEST_CASE("tree_node has size one if it is initialized with starter kv") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj, lit::new_node_tag{}, Test_Key, Test_Value};
    CHECK(sut.size() == 1);
}

TEST_CASE("tree_node's dump contains payload's dump") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};
    const payload_type payload;

    std::ostringstream ss;
    sut.dump(ss);
    auto sut_dump = ss.str();
    ss.str("");
    ss << payload;
    auto pl_dump = ss.str();

    CHECK_FALSE(sut_dump.find(pl_dump) == std::string::npos);
}

TEST_CASE("empty tree_node's search returns nullopt") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj};

    auto res = sut.search(Test_Key);
    CHECK(std::get<std::optional<int>>(res) == std::nullopt);
}

TEST_CASE("leaf tree_node with key to left returns nullopt") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj, lit::new_node_tag{}, Test_Key, Test_Value};
    auto res = sut.search(Test_Key3);
    CHECK(std::get<std::optional<int>>(res) == std::nullopt);
}

TEST_CASE("leaf tree_node with key to right returns nullopt") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    const sut_type sut{obj, lit::new_node_tag{}, Test_Key, Test_Value};
    auto res = sut.search(Test_Key2);
    CHECK(std::get<std::optional<int>>(res) == std::nullopt);
}

TEST_CASE("empty tree_node's insert returns nullptr (success)") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    auto* obj = &mock.get();
    sut_type sut{obj};

    const auto* res = sut.insert(Test_Key, Test_Value);
    CHECK(res == nullptr);
}

TEST_CASE("full tree_node's insert returns nullptr if it was a leaf") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    When(Method(mock, increment_side_of_child)).AlwaysReturn();
    auto* obj = &mock.get();
    sut_type sut{obj, lit::new_node_tag{}, Test_Key, Test_Value};

    const auto* res = sut.insert(Test_Key2, Test_Value);
    CHECK(res == nullptr);
    Verify(Method(mock, increment_side_of_child).Using(&sut)).Once();
}

TEST_CASE("full tree_node's insert larger element makes the node right-heavy") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    When(Method(mock, increment_side_of_child)).AlwaysReturn();
    auto* obj = &mock.get();
    sut_type sut{obj, lit::new_node_tag{}, Test_Key, Test_Value};

    std::ignore = sut.insert(Test_Key2, Test_Value);
    CHECK(sut.balance_factor() == 1);
}

TEST_CASE("full tree_node's insert smaller element makes the node right-heavy") {
    Mock<lit::tree_node_handler<sut_type>> mock;
    When(Method(mock, increment_side_of_child)).AlwaysReturn();
    auto* obj = &mock.get();
    sut_type sut{obj, lit::new_node_tag{}, Test_Key, Test_Value};

    std::ignore = sut.insert(Test_Key3, Test_Value);
    CHECK(sut.balance_factor() == -1);
}

