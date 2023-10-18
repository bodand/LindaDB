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
 * test/LindaDB/tree/avl_tree --
 *   Tests that perform tests against a tree implementation that behaves as a
 *   simple AVL-Tree with scalar payloads.
 */

#include <algorithm>
#include <random>
#include <ranges>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/tree.hxx>

namespace lit = ldb::index::tree;
namespace lps = lit::payloads;
using sut_type = lit::tree<int, int, 1>;
using bm_type = lit::tree<int, int, 1>;

// ordering of test keys: Test_Key3 < Test_Key < Test_Key2
// the test suite relies on this ordering, so even if
// modifying the keys retain this property
constexpr const static auto Test_Key = 42;
constexpr const static auto Test_Key2 = 420;
constexpr const static auto Test_Key3 = 7;
constexpr const static auto Test_Value = 42;

TEST_CASE("avl tree can be default constructed") {
    STATIC_CHECK(std::constructible_from<sut_type>);
}

//TEST_CASE("default avl tree has height 0") {
//    const sut_type sut;
//    CHECK(sut.height() == 0);
//}
//
//TEST_CASE("avl tree with one element has height 1") {
//    sut_type sut;
//    sut.insert(Test_Key, Test_Value);
//    CHECK(sut.height() == 1);
//}
//
//TEST_CASE("avl tree with three elements has height 2 (inserted increasing)") {
//    /* Rotation test:
//     *        7                    42
//     *         \                  /  \
//     *          42        -->    7   420
//     *            \
//     *            420
//     * */
//    sut_type sut;
//    // this order would cause a simple binary tree to become a list
//    sut.insert(Test_Key3, Test_Value);
//    sut.insert(Test_Key, Test_Value);
//    sut.insert(Test_Key2, Test_Value);
//    CHECK(sut.height() == 2);
//}
//
//TEST_CASE("avl tree with three elements has height 2 (inserted decreasing)") {
//    /* Rotation test:
//     *        420             42
//     *        /              /  \
//     *       42      -->    7   420
//     *      /
//     *     7
//     * */
//    sut_type sut;
//    // this order would cause a simple binary tree to become a list
//    sut.insert(Test_Key2, Test_Value);
//    sut.insert(Test_Key, Test_Value);
//    sut.insert(Test_Key3, Test_Value);
//    CHECK(sut.height() == 2);
//}
//
//TEST_CASE("avl tree with three elements has height 2 (inserted correct order)") {
//    /* no rotation test: there should be no rotation happening here */
//    sut_type sut;
//    sut.insert(Test_Key, Test_Value);
//    sut.insert(Test_Key3, Test_Value);
//    sut.insert(Test_Key2, Test_Value);
//    CHECK(sut.height() == 2);
//}
//
//TEST_CASE("avl tree with five elements has height 3 (left side)") {
//    /* Rotation test:
//     *         42                  42
//     *        /  \                /  \
//     *       8   420             7   420
//     *      /           -->     / \
//     *     6                   6   8
//     *      \
//     *       7
//     * */
//    sut_type sut;
//    sut.insert(Test_Key, Test_Value);
//    sut.insert(Test_Key2, Test_Value);
//    sut.insert(Test_Key3 + 1, Test_Value);
//    sut.insert(Test_Key3 - 1, Test_Value);
//    sut.insert(Test_Key3, Test_Value);
//    CHECK(sut.height() == 3);
//}
//
//TEST_CASE("avl tree with five elements has height 3 (right side)") {
//    /* Rotation test:
//     *         42                  42
//     *        /  \                /  \
//     *       7   419             7   420
//     *             \     -->         / \
//     *             421             419 421
//     *             /
//     *           420
//     * */
//    sut_type sut;
//    sut.insert(Test_Key, Test_Value);
//    sut.insert(Test_Key3, Test_Value);
//    sut.insert(Test_Key2 - 1, Test_Value);
//    sut.insert(Test_Key2 + 1, Test_Value);
//    sut.insert(Test_Key2, Test_Value);
//    CHECK(sut.height() == 3);
//}
//
//TEST_CASE("avl tree can find stored element") {
//    sut_type sut;
//    sut.insert(Test_Key, Test_Value);
//    sut.insert(Test_Key3, Test_Value);
//    sut.insert(Test_Key2 - 1, Test_Value);
//    sut.insert(Test_Key2 + 1, Test_Value);
//    sut.insert(Test_Key2, Test_Value);
//
//    auto val = sut.search(Test_Key2);
//    REQUIRE(val != std::nullopt);
//    CHECK(*val == Test_Value);
//}
//
//TEST_CASE("avl tree can find updated element") {
//    sut_type sut;
//    sut.insert(Test_Key, Test_Value + 1);
//    sut.insert(Test_Key3, Test_Value);
//    sut.insert(Test_Key2 - 1, Test_Value);
//    sut.insert(Test_Key2 + 1, Test_Value);
//    sut.insert(Test_Key2, Test_Value);
//    sut.insert(Test_Key, Test_Value);
//
//    auto val = sut.search(Test_Key2);
//    REQUIRE(val != std::nullopt);
//    CHECK(*val == Test_Value);
//}
//
//TEST_CASE("avl-tree benchmark",
//          "[.benchmark]") {
//    BENCHMARK_ADVANCED("avl-tree insertion/empty tree")
//    (Catch::Benchmark::Chronometer chronometer) {
//        bm_type bm;
//        chronometer.measure([&bm](int i) {
//            bm.insert(i, Test_Value);
//        });
//    };
//    BENCHMARK_ADVANCED("avl-tree insertion/full layer")
//    (Catch::Benchmark::Chronometer chronometer) {
//        bm_type bm;
//        std::mt19937_64 rng(std::random_device{}());
//        std::uniform_int_distribution<int> dist(0, chronometer.runs());
//        for (std::size_t i = 0; i < bm.node_capacity(); ++i) {
//            bm.insert(dist(rng), static_cast<int>(i));
//        }
//        chronometer.measure([&bm](int i) {
//            bm.insert(i, Test_Value);
//        });
//    };
//    BENCHMARK_ADVANCED("avl-tree insertion/random (has rng overhead)")
//    (Catch::Benchmark::Chronometer chronometer) {
//        bm_type bm;
//        std::mt19937_64 rng(std::random_device{}());
//        std::uniform_int_distribution<int> dist(0, chronometer.runs());
//        for (std::size_t i = 0; i < bm.node_capacity() * 8; ++i) {
//            bm.insert(dist(rng), static_cast<int>(i));
//        }
//        std::vector<int> data(chronometer.runs());
//        std::ranges::generate(data, [&dist, &rng]() {
//            return dist(rng);
//        });
//        chronometer.measure([&bm, &data](int i) {
//            bm.insert(data[i], Test_Value);
//        });
//    };
//}
