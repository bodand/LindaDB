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
 * Originally created: 2023-11-08.
 *
 * test/LindaRT/serialize_tuple --
 *   Tests for serialization of tuples.
 */

#include <array>
#include <concepts>
#include <cstddef>
#include <string>
#include <vector>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <ldb/lv/linda_tuple.hxx>
#include <lrt/serialize/tuple.hxx>

namespace lv = ldb::lv;

TEST_CASE("empty tuple serializes") {
    const lv::linda_tuple empty;
    auto ser = lrt::serialize(empty);
    const auto& [serial, serial_sz] = ser;

    std::array<std::byte, sizeof(std::size_t) + 1> exp = {
           static_cast<std::byte>(0b01),
           static_cast<std::byte>(0b00),
    };
    CHECK(serial_sz == exp.size());
    CHECK(std::equal(serial.get(), serial.get() + serial_sz, exp.begin()));
}

TEST_CASE("empty tuple deserializes") {
    const lv::linda_tuple empty;
    auto ser = lrt::serialize(empty);
    const auto& [serial, serial_sz] = ser;
    auto got = lrt::deserialize({serial.get(), serial_sz});
    CHECK(empty == got);
}

TEST_CASE("tuple with numbers serializes",
          "[little-endian]") {
    const lv::linda_tuple t(42, 69);
    auto ser = lrt::serialize(t);
    const auto& [serial, serial_sz] = ser;

    constexpr const static auto int_typemap = std::byte{1};

    std::vector<std::byte> exp(sizeof(std::size_t) + (sizeof(int) + 1) * 2 + 1);
    exp[0] = std::byte{1};
    exp[1] = std::byte{2};
    exp[sizeof(std::size_t) + 1] = int_typemap;
    exp[sizeof(std::size_t) + 1 + 1] = static_cast<std::byte>(std::get<int>(t[0]));
    exp[sizeof(std::size_t) + 1 + sizeof(int) + 1] = int_typemap;
    exp[sizeof(std::size_t) + 1 + sizeof(int) + 1 + 1] = static_cast<std::byte>(std::get<int>(t[1]));
    CHECK(serial_sz == exp.size());
    CHECK_THAT(exp, Catch::Matchers::RangeEquals(std::span{serial.get(), serial.get() + serial_sz}));
}

TEST_CASE("tuple with numbers deserializes") {
    const lv::linda_tuple t(42, 69);
    auto ser = lrt::serialize(t);
    const auto& [serial, serial_sz] = ser;
    auto got = lrt::deserialize({serial.get(), serial_sz});
    CHECK(t == got);
}

TEST_CASE("tuple with string serializes",
          "[little-endian]") {
    const lv::linda_tuple t("asd", 2, "xy");
    auto ser = lrt::serialize(t);
    const auto& [serial, serial_sz] = ser;

    constexpr const static auto int_typemap = std::byte{1};
    constexpr const static auto str_typemap = std::byte{6};

    std::vector<std::byte> exp(1
                               + sizeof(std::size_t)
                               + 1 + sizeof(std::size_t) + 3
                               + 1 + sizeof(int)
                               + 1 + sizeof(std::size_t) + 2);
    exp[0] = std::byte{1};
    exp[1] = std::byte{3};
    exp[sizeof(std::size_t) + 1] = str_typemap;
    exp[sizeof(std::size_t) + 1 + 1] = std::byte{3};
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 1] = std::byte{'a'};
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 2] = std::byte{'s'};
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 3] = std::byte{'d'};
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 3 + 1] = int_typemap;
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 3 + 1 + 1] = std::byte{2};
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 3 + 1 + sizeof(int) + 1] = str_typemap;
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 3 + 1 + sizeof(int) + 1 + 1] = std::byte{2};
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 3 + 1 + sizeof(int) + 1 + sizeof(std::size_t) + 1] = std::byte{'x'};
    exp[sizeof(std::size_t) + 1 + sizeof(std::size_t) + 3 + 1 + sizeof(int) + 1 + sizeof(std::size_t) + 2] = std::byte{'y'};
    CHECK(serial_sz == exp.size());
    CHECK_THAT(exp, Catch::Matchers::RangeEquals(std::span{serial.get(), serial.get() + serial_sz}));
}

TEST_CASE("tuple with string deserializes") {
    const lv::linda_tuple t("asd", 2, "hello world!");
    auto ser = lrt::serialize(t);
    const auto& [serial, serial_sz] = ser;
    auto got = lrt::deserialize({serial.get(), serial_sz});
    CHECK(t == got);
}

template<class T>
struct instantiate {
    constexpr static auto value = T{};
};

template<std::integral T>
struct instantiate<T> {
    constexpr static auto value = T{42};
};

template<std::floating_point T>
struct instantiate<T> {
    constexpr static auto value = T{static_cast<T>(4.2)};
};

template<>
struct instantiate<std::string> {
    inline static auto value = std::string{"42xx"};
};

template<>
struct instantiate<lv::fn_call_holder> {
    inline static auto value = lv::fn_call_holder{"fn_name", std::make_unique<lv::linda_tuple>(1)};
};

template<>
struct instantiate<lv::ref_type> {
    inline static auto value = lv::ref_type(std::size_t{1});
};

TEMPLATE_LIST_TEST_CASE("tuple with all fields can round trip serialization",
                        "[serialize][deserialize]",
                        lv::linda_value) {
    const auto payload = instantiate<TestType>::value;

    const lv::linda_tuple t(payload);
    const auto [serial, serial_sz] = lrt::serialize(t);
    CHECK(serial_sz > sizeof(std::size_t) + 1);
    const auto back = lrt::deserialize({serial.get(), serial_sz});
    CHECK(t == back);
}
