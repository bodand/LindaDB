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
 * Originally created: 2024-01-12.
 *
 * test/LindaDB/lv/linda_value --
 *   Test for the lv::linda_value type and associated operations.
 */

#include <cstdint>
#include <sstream>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>

namespace lv = ldb::lv;

TEMPLATE_TEST_CASE("linda_value prints as integer value",
                   "[linda_value]",
                   std::int16_t,
                   std::uint16_t,
                   std::int32_t,
                   std::uint32_t,
                   std::int64_t,
                   std::uint64_t) {
    using namespace lv::io;
    const lv::linda_value val = static_cast<TestType>(42);
    std::ostringstream oss;
    oss << val;
    CHECK(oss.str() == "42");
}

TEMPLATE_TEST_CASE("linda_value prints as float value",
                   "[linda_value]",
                   float,
                   double) {
    using namespace lv::io;
    const lv::linda_value val = static_cast<TestType>(42.12);
    std::ostringstream oss;
    oss << val;
    CHECK(oss.str().substr(0, 5) == "42.12");
}

TEST_CASE("linda_value prints as string value",
          "[linda_value]") {
    using namespace lv::io;
    using namespace std::literals;
    const auto string_value = "some string"s;

    const lv::linda_value val = string_value;
    std::ostringstream oss;
    oss << val;
    CHECK(oss.str() == string_value);
}

TEST_CASE("make_linda_value constructs non-string value",
          "[linda_value]") {
    using namespace std::literals;

    CHECK(lv::linda_value(1) == lv::make_linda_value(1));
    CHECK(lv::linda_value(1.f) == lv::make_linda_value(1.f));
    CHECK(lv::linda_value(1.) == lv::make_linda_value(1.));
    CHECK(lv::linda_value(lv::fn_call_tag{}) == lv::make_linda_value(lv::fn_call_tag{}));
    CHECK(lv::linda_value(lv::fn_call_holder{"a", std::make_unique<lv::linda_tuple>()}) == lv::make_linda_value(lv::fn_call_holder{"a", std::make_unique<lv::linda_tuple>()}));
}

TEST_CASE("make_linda_value constructs string value",
          "[linda_value]") {
    using namespace std::literals;
    const auto cstring_value = "some string";
    const auto string_value = "some string"s;
    const auto string_view_value = "some string"sv;

    SECTION("c-string") {
        CHECK(lv::linda_value(string_value) == lv::make_linda_value(cstring_value));
    }

    SECTION("std::string") {
        CHECK(lv::linda_value(string_value) == lv::make_linda_value(string_value));
    }

    SECTION("std::string_view") {
        CHECK(lv::linda_value(string_value) == lv::make_linda_value(string_view_value));
    }
}
