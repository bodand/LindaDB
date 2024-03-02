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
 * Originally created: 2024-02-22.
 *
 * test/LindaDB/lv/tuple_builder --
 *   Tests for the tuple builder class.
 */


#include <variant>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/tuple_builder.hxx>

#include "ldb/lv/fn_call_holder.hxx"

using namespace ldb;

namespace {
    int
    zero_tuple_builder(int, const char*) { return 0; }
}

TEST_CASE("empty tuple builder builds empty tuple") {
    auto res = lv::tuple_builder().build();
    CHECK(res == lv::linda_tuple());
}

TEST_CASE("tuple builder with skipped initial build step builds correct tuple") {
    auto res = lv::tuple_builder()("", 1)("", 2).build();
    CHECK(res == lv::linda_tuple(1, 2));
}

TEST_CASE("tuple builder builds single value") {
    auto res = lv::tuple_builder("ignored", 1).build();
    CHECK(res == lv::linda_tuple(1));
}

TEST_CASE("tuple builder builds chained values") {
    auto res = lv::tuple_builder("1", 1)("2", 2)("\"asd\"", "asd").build();
    CHECK(res == lv::linda_tuple(1, 2, "asd"));
}

TEST_CASE("tuple builder builds with function calls") {
    auto res = lv::tuple_builder("zero", zero_tuple_builder)("2, \"yeet\"", 2, "yeet").build();
    auto* sut = std::get_if<lv::fn_call_holder>(&res[0]);
    REQUIRE_FALSE(sut == nullptr);
    CHECK(sut->fn_name() == "zero");
    CHECK(sut->args() == lv::linda_tuple(2, "yeet"));
}
