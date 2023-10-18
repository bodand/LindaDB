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
 * test/LindaDB/tree/t_tree --
 *   Tests that perform tests against a tree implementation that behaves as a
 *   simple AVL-Tree with scalar payloads.
 */
#include <random>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/payload.hxx>
#include <ldb/index/tree/payload/scalar_payload.hxx>
#include <ldb/index/tree/tree.hxx>
#include <ldb/index/tree/tree_node.hxx>
#include <ldb/index/tree/tree_node_handler.hxx>

namespace lit = ldb::index::tree;
namespace lps = lit::payloads;
using payload_type = lps::scalar_payload<int, int>;
using sut_type = lit::tree<int, int, 2>;
using bm_type = lit::tree<int, int>;

// ordering of test keys: Test_Key3 < Test_Key < Test_Key2
// do not change these, the rotation test explanations use these values
constexpr const static auto Test_Key = 42;
constexpr const static auto Test_Key2 = 420;
constexpr const static auto Test_Key3 = 7;
constexpr const static auto Test_Value = 42;

TEST_CASE("t tree can be default constructed") {
    STATIC_CHECK(std::constructible_from<sut_type>);
}

TEST_CASE("default t tree has height 0") {
    const sut_type sut;
    CHECK(sut.dump_string() == R"_x_(((2 0) 0
  ()
  ())
)_x_");
}

TEST_CASE("t tree with one element has height 1") {
    sut_type sut;
    sut.insert(Test_Key, Test_Value);
    CHECK(sut.dump_string() == R"_x_(((2 1 (42 42)) 0
  ()
  ())
)_x_");
}

TEST_CASE("t tree with five elements has height 2 (inserted increasing)") {
    /* Rotation test:
     *        [7, 8]                    [42, 43]
     *             \                    /      \
     *           [42, 43]       -->  [7, 8]   [420]
     *                  \
     *                 [420]
     * */
    sut_type sut;
    // this order would cause a simple binary tree to become a list
    sut.insert(Test_Key3, Test_Value);
    sut.insert(Test_Key3 + 1, Test_Value);
    sut.insert(Test_Key, Test_Value);
    sut.insert(Test_Key + 1, Test_Value);
    sut.insert(Test_Key2, Test_Value);
    CHECK(sut.dump_string() == R"_x_(((2 2 (42 42) (43 42)) 0
  ((2 2 (7 42) (8 42)) 0
    ()
    ())
  ((2 1 (420 42)) 0
    ()
    ()))
)_x_");
}

TEST_CASE("t tree with five elements has height 2 (inserted decreasing)") {
    /* Rotation test:
     *        [420, 421]          [42, 43]
     *        /                   /      \
     *   [42, 43]       -->      [7]  [420, 421]
     *   /
     *  [7]
     * */
    sut_type sut;
    // this order would cause a simple binary tree to become a list
    sut.insert(Test_Key2, Test_Value);
    sut.insert(Test_Key2 + 1, Test_Value);
    sut.insert(Test_Key, Test_Value);
    sut.insert(Test_Key + 1, Test_Value);
    sut.insert(Test_Key3, Test_Value);
    CHECK(sut.dump_string() == R"_x_(((2 2 (42 42) (43 42)) 0
  ((2 1 (7 42)) 0
    ()
    ())
  ((2 2 (420 42) (421 42)) 0
    ()
    ()))
)_x_");
}

TEST_CASE("t tree with three elements has height 2 (inserted correct order)") {
    /* no rotation test: there should be no rotation happening here */
    sut_type sut;
    sut.insert(Test_Key, Test_Value);
    sut.insert(Test_Key3, Test_Value);
    sut.insert(Test_Key2, Test_Value);
    CHECK(sut.dump_string() == R"_x_(((2 2 (7 42) (42 42)) 1
  ()
  ((2 1 (420 42)) 0
    ()
    ()))
)_x_");
}

TEST_CASE("t tree with eight elements has height 3 (left side)") {
    /* Rotation test:
     *         [42, 43]                 [42, 43]
     *         /      \                 /      \
     *      [8, 9]   [420]           [  7  ]  [420]
     *      /              -->       /     \
     *   [5, 6]                   [5, 6] [8, 9]
     *        \
     *       [7]
     * */
    sut_type sut;
    sut.insert(Test_Key, Test_Value);
    sut.insert(Test_Key + 1, Test_Value);
    sut.insert(Test_Key2, Test_Value);
    sut.insert(Test_Key3 + 1, Test_Value);
    sut.insert(Test_Key3 + 2, Test_Value);
    sut.insert(Test_Key3 - 1, Test_Value);
    sut.insert(Test_Key3 - 2, Test_Value);
    sut.insert(Test_Key3, Test_Value);
    CHECK(sut.dump_string() == R"_x_(((2 2 (42 42) (43 42)) -1
  ((2 1 (7 42)) 0
    ((2 2 (5 42) (6 42)) 0
      ()
      ())
    ((2 2 (8 42) (9 42)) 0
      ()
      ()))
  ((2 1 (420 42)) 0
    ()
    ()))
)_x_");
}

TEST_CASE("t tree with eight elements has height 3 (right side)") {
    /* Rotation test:
     *      [42, 43]                  [  42, 43  ]
     *      /      \                  /          \
     *     [7]  [418, 419]           [7]   [    420    ]
     *                   \     -->         /           \
     *                [421, 422]      [418, 419]   [421, 422]
     *                /
     *              [420]
     * */
    sut_type sut;
    sut.insert(Test_Key, Test_Value);
    sut.insert(Test_Key + 1, Test_Value);
    sut.insert(Test_Key3, Test_Value);
    sut.insert(Test_Key2 - 1, Test_Value);
    sut.insert(Test_Key2 - 2, Test_Value);
    sut.insert(Test_Key2 + 1, Test_Value);
    sut.insert(Test_Key2 + 2, Test_Value);
    sut.insert(Test_Key2, Test_Value);
    CHECK(sut.dump_string() == R"_x_(((2 2 (42 42) (43 42)) 1
  ((2 1 (7 42)) 0
    ()
    ())
  ((2 1 (420 42)) 0
    ((2 2 (418 42) (419 42)) 0
      ()
      ())
    ((2 2 (421 42) (422 42)) 0
      ()
      ())))
)_x_");
}

TEST_CASE("t tree can find stored element") {
    /*
     *      [   419   ]
     *      /         \
     *   [7, 42]  [420, 421]
     * */
    sut_type sut;
    sut.insert(Test_Key, Test_Value);
    sut.insert(Test_Key3, Test_Value);
    sut.insert(Test_Key2 - 1, Test_Value);
    sut.insert(Test_Key2 + 1, Test_Value);
    sut.insert(Test_Key2, Test_Value);

    auto val = sut.search(Test_Key2);
    REQUIRE(val != std::nullopt);
    CHECK(*val == Test_Value);
}

TEST_CASE("t tree can find updated element") {
    sut_type sut;
    sut.insert(Test_Key, Test_Value + 1);
    sut.insert(Test_Key3, Test_Value);
    sut.insert(Test_Key2 - 1, Test_Value);
    sut.insert(Test_Key2 + 1, Test_Value);
    sut.insert(Test_Key2, Test_Value);
    sut.insert(Test_Key, Test_Value);

    auto val = sut.search(Test_Key2);
    REQUIRE(val != std::nullopt);
    CHECK(*val == Test_Value);
}

TEST_CASE("t tree takes random data") {
    sut_type sut;
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 12000);
    auto first = dist(rng);
    sut.insert(dist(rng), 0);
    for (std::size_t i = 0; i < 1'999'999; ++i) {
        sut.insert(dist(rng), static_cast<int>(i));
    }
    CHECK(sut.search(first) != std::nullopt);
}

TEST_CASE("t-tree benchmark",
          "[.benchmark]") {
    BENCHMARK_ADVANCED("t-tree insertion/empty tree")
    (Catch::Benchmark::Chronometer chronometer) {
        bm_type bm;
        chronometer.measure([&bm](int i) {
            bm.insert(i, Test_Value);
        });
    };
    BENCHMARK_ADVANCED("t-tree insertion/full layer")
    (Catch::Benchmark::Chronometer chronometer) {
        bm_type bm;
        std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, chronometer.runs());
        for (std::size_t i = 0; i < bm.node_capacity(); ++i) {
            bm.insert(dist(rng), static_cast<int>(i));
        }
        chronometer.measure([&bm](int i) {
            bm.insert(i, Test_Value);
        });
    };
    BENCHMARK_ADVANCED("t-tree insertion/random (has rng overhead)")
    (Catch::Benchmark::Chronometer chronometer) {
        bm_type bm;
        std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, chronometer.runs());
        for (std::size_t i = 0; i < bm.node_capacity() * 8; ++i) {
            bm.insert(dist(rng), static_cast<int>(i));
        }
        std::vector<int> data(chronometer.runs());
        std::ranges::generate(data, [&dist, &rng]() {
            return dist(rng);
        });
        chronometer.measure([&bm, &data](int i) {
            bm.insert(data[i], Test_Value);
        });
    };
}
