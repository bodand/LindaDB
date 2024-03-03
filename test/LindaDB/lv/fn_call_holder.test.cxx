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
 * Originally created: 2024-02-27.
 *
 * test/LindaDB/lv/fn_call_holder --
 *   
 */

#include <cstring>
#include <memory>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/dyn_function_adapter.hxx>
#include <ldb/lv/fn_call_holder.hxx>
#include <ldb/lv/global_function_map.hxx>
#include <ldb/lv/linda_tuple.hxx>

using namespace ldb;
using namespace std::literals;

namespace {
    int
    zero_fn_call_holder(int x, const char* str) { return static_cast<int>(std::strlen(str)) + x; }
}

TEST_CASE("fn_call_holder retains function's name") {
    const auto* fn_name = "zero_fn_call_holder";
    const lv::fn_call_holder fn(fn_name, std::make_unique<lv::linda_tuple>(42, "asd"));
    CHECK(fn.fn_name() == fn_name);
}

TEST_CASE("fn_call_holder retains call arguments") {
    auto tuple_ptr = std::make_unique<lv::linda_tuple>(42, "asd");
    const lv::fn_call_holder fn("zero_fn_call_holder", tuple_ptr->clone());
    CHECK(fn.args() == *tuple_ptr);
}

TEST_CASE("fn_call_holder is copy constructible") {
    auto tuple_ptr = std::make_unique<lv::linda_tuple>(42, "asd");
    const lv::fn_call_holder fn("zero_fn_call_holder", tuple_ptr->clone());
    const auto copy = fn;
    CHECK(copy == fn);
    CHECK(copy.args() == fn.args());
}

TEST_CASE("fn_call_holder is copy assignable") {
    auto tuple_ptr = std::make_unique<lv::linda_tuple>(42, "asd");
    const lv::fn_call_holder fn("zero_fn_call_holder", tuple_ptr->clone());
    lv::fn_call_holder fn2("bad", std::make_unique<lv::linda_tuple>());
    fn2 = fn;
    CHECK(fn2 == fn);
    CHECK(fn2.args() == fn.args());
}

TEST_CASE("fn_call_holder is move constructible") {
    auto tuple_ptr = std::make_unique<lv::linda_tuple>(42, "asd");
    lv::fn_call_holder fn("zero_fn_call_holder", tuple_ptr->clone());
    const auto copy = fn;
    const auto sut = std::move(fn);
    CHECK(copy == sut);
    CHECK(copy.args() == sut.args());
}

TEST_CASE("fn_call_holder is move assignable") {
    auto tuple_ptr = std::make_unique<lv::linda_tuple>(42, "asd");
    lv::fn_call_holder fn("zero_fn_call_holder", tuple_ptr->clone());
    lv::fn_call_holder copy = fn;
    lv::fn_call_holder sut("bad", std::make_unique<lv::linda_tuple>());
    sut = std::move(fn);
    CHECK(copy == sut);
    CHECK(copy.args() == sut.args());
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
TEST_CASE("fn_call_holder handles self-assignment") {
    auto tuple_ptr = std::make_unique<lv::linda_tuple>(42, "asd");
    lv::fn_call_holder fn("zero_fn_call_holder", tuple_ptr->clone());
    fn = fn;
    CHECK(fn == fn);
    CHECK(fn.args() == fn.args());
}
#pragma clang diagnostic pop

TEST_CASE("fn_call_holder's function can be called") {
    ldb::lv::gLdb_Dynamic_Function_Map = std::make_unique<ldb::lv::global_function_map_type>();
    lv::gLdb_Dynamic_Function_Map->try_emplace("zero_fn_call_holder", [](const ldb::lv::linda_tuple& args) -> ldb::lv::linda_value {
        ldb::lv::dyn_function_adapter adapter(zero_fn_call_holder);
        return adapter(args);
    });

    lv::fn_call_holder fn("zero_fn_call_holder", std::make_unique<lv::linda_tuple>(42, "asd"));
    auto res = fn.execute();
    CHECK(res == lv::linda_tuple(45));
}

TEST_CASE("fn_call_holder hashes to the same value as its name") {
    std::hash<lv::fn_call_holder> fn_holder_hasher;
    std::hash<std::string> str_hasher;

    const auto fn_name_str = "zero_fn_call_holder"s;
    lv::fn_call_holder fn(fn_name_str, std::make_unique<lv::linda_tuple>(42, "asd"));

    CHECK(fn_holder_hasher(fn) == str_hasher(fn_name_str));
}

TEST_CASE("fn_call_holder prints its held function name") {
    const auto fn_name_str = "zero_fn_call_holder"s;
    lv::fn_call_holder fn(fn_name_str, std::make_unique<lv::linda_tuple>(42, "asd"));
    std::ostringstream oss;
    oss << fn;
    CHECK(oss.str().contains(fn_name_str));
}

TEST_CASE("fn_call_holder equals based on fn_name") {
    SECTION("equal names, equal parameters") {
        const auto fn_name_str = "zero_fn_call_holder"s;
        lv::fn_call_holder fn1(fn_name_str, std::make_unique<lv::linda_tuple>(42, "asd"));
        lv::fn_call_holder fn2(fn_name_str, std::make_unique<lv::linda_tuple>(42, "asd"));
        CHECK(fn1 == fn2);
    }

    SECTION("equal names, diff parameters") {
        const auto fn_name_str = "zero_fn_call_holder"s;
        lv::fn_call_holder fn1(fn_name_str, std::make_unique<lv::linda_tuple>(42));
        lv::fn_call_holder fn2(fn_name_str, std::make_unique<lv::linda_tuple>("asd"));
        CHECK(fn1 == fn2);
    }

    SECTION("diff names, diff parameters") {
        lv::fn_call_holder fn1("zero_fn_call_holder"s, std::make_unique<lv::linda_tuple>(42, "asd"));
        lv::fn_call_holder fn2("one_fn_call_holder"s, std::make_unique<lv::linda_tuple>(421, "asd"));
        CHECK_FALSE(fn1 == fn2);
    }

    SECTION("diff names, diff parameters") {
        lv::fn_call_holder fn1("zero_fn_call_holder"s, std::make_unique<lv::linda_tuple>(42));
        lv::fn_call_holder fn2("one_fn_call_holder"s, std::make_unique<lv::linda_tuple>("asd"));
        CHECK_FALSE(fn1 == fn2);
    }
}

TEST_CASE("fn_call_holder compare based on fn_name") {
    lv::fn_call_holder fn0("0", std::make_unique<lv::linda_tuple>());
    lv::fn_call_holder fn1("1", std::make_unique<lv::linda_tuple>());
    lv::fn_call_holder fn2("2", std::make_unique<lv::linda_tuple>());

    CHECK(fn0 < fn1);
    CHECK_FALSE(fn1 < fn0);
    CHECK(fn2 > fn1);
    CHECK_FALSE(fn1 > fn2);
}
