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
 * Originally created: 2023-10-29.
 *
 * test/LindaDB/store --
 *   Tests for the main store object representing a full IMDB instance.
 */


#include <array>
#include <fstream>
#include <iomanip>
#include <latch>
#include <mutex>
#include <ostream>
#include <random>
#include <string_view>
#include <thread>
#include <tuple>

#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query_tuple.hxx>
#include <ldb/store.hxx>
#include "ldb/bcast/broadcaster.hxx"

namespace lv = ldb::lv;
using namespace std::literals;

TEST_CASE("store is default constructible") {
    STATIC_CHECK(std::constructible_from<ldb::store>);
}

TEST_CASE("store can store and rdp by value a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);
    auto ret = store.rdp(ldb::query_tuple("asd", 2));
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store without signaling and rdp by value a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out_nosignal(tuple);
    auto ret = store.rdp(ldb::query_tuple("asd", 2));
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and rd by value a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);
    auto ret = store.rd(ldb::query_tuple("asd", 2));
    CHECK(ret == tuple);
}

TEST_CASE("store can store and rdp by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.rdp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           1));
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store for rdp of non-existent tuple by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.rdp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           0));
    REQUIRE_FALSE(ret.has_value());
}

TEST_CASE("store can store and rdp by type a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    int val = 0;
    auto ret = store.rdp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(lv::linda_value(val) == tuple[1]);
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and rdp cannot retrieve tuple with mismatched type") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    std::int64_t val = 0LL;
    auto ret = store.rdp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(val == 0LL);
    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store can store and rdp cannot retrieve tuple with mismatched value") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    auto ret = store.rdp(ldb::query_tuple("asd", 3));

    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store can repeat rdp calls for existing tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    for (int i = 0; i < 15; ++i) {
        int val = 0;
        auto ret = store.rdp(ldb::query_tuple("asd", ldb::ref(&val)));

        CHECK(lv::linda_value(val) == tuple[1]);
        REQUIRE(ret.has_value());
        CHECK(*ret == tuple);
    }
}

TEST_CASE("store can repeat rdp calls for missing tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    for (int i = 0; i < 15; ++i) {
        auto ret = store.rdp(ldb::query_tuple("asd", 3));
        CHECK_FALSE(ret.has_value());
    }
}

TEST_CASE("store can store and inp by value a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);
    auto ret = store.inp(ldb::query_tuple("asd", 2));
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and inp by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.inp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           1));
    REQUIRE(ret.has_value());
}

TEST_CASE("store for inp of non-existent tuple by value a nonempty tuple without index") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 5, 4, 3, 2, 1);
    store.out(tuple);
    std::string name;
    int var1;
    int var2;
    int var3;
    int var4;
    auto ret = store.inp(ldb::query_tuple(
           ldb::ref(&name), // asd
           ldb::ref(&var1), // 5
           ldb::ref(&var2), // 4
           ldb::ref(&var3), // 3
           ldb::ref(&var4), // 2
           0));
    REQUIRE_FALSE(ret.has_value());
}

TEST_CASE("store can store and inp by type a nonempty tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    int val = 0;
    auto ret = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(lv::linda_value(val) == tuple[1]);
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
}

TEST_CASE("store can store and inp cannot retrieve tuple with mismatched type") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    std::int64_t val = 0LL;
    auto ret = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));

    CHECK(val == 0LL);
    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store can store and inp cannot retrieve tuple with mismatched value") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    auto ret = store.inp(ldb::query_tuple("asd", 3));

    CHECK_FALSE(ret.has_value());
}

TEST_CASE("store cannot repeat inp calls for existing tuple: only first succeeds") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    int val = 0;
    auto ret = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));
    CHECK(lv::linda_value(val) == tuple[1]);
    REQUIRE(ret.has_value());
    CHECK(*ret == tuple);
    auto ret2 = store.inp(ldb::query_tuple("asd", ldb::ref(&val)));
    CHECK_FALSE(ret2.has_value());
}

TEST_CASE("store can repeat inp calls for missing tuple") {
    ldb::store store;
    auto tuple = lv::linda_tuple("asd", 2);
    store.out(tuple);

    for (int i = 0; i < 15; ++i) {
        auto ret = store.inp(ldb::query_tuple("asd", 3));
        CHECK_FALSE(ret.has_value());
    }
}

TEST_CASE("store can store zero length tuples") {
    ldb::store store;
    const auto tuple = lv::linda_tuple();
    store.out(tuple);
    const auto found = store.in(ldb::query_tuple());
    CHECK(found == tuple);
}

TEST_CASE("store does not deadlock trivially when out is called on a waiting in") {
    static std::uniform_int_distribution<unsigned> time_dist(500'000U, 1'000'000U);
    static std::uniform_int_distribution<int> val_dist(100'000, 300'000);
    static std::mt19937_64 rng(std::random_device{}());
    constexpr const static auto repeat_count = 1;
    ldb::store store;
    int rand{};
    auto query = ldb::query_tuple("asd", ldb::ref(&rand));

    const auto adder = std::jthread([&store]() {
        for (int i = 0; i < repeat_count; ++i) {
            auto val = lv::linda_tuple("asd", val_dist(rng));
            std::this_thread::sleep_for(std::chrono::nanoseconds(time_dist(rng)));
            store.out(val);
        }
    });

    for (int i = 0; i < repeat_count; ++i) {
        auto ret = store.in(query);
        CHECK(ret[0] == lv::linda_value("asd"));
        CHECK(rand >= 100'000);
        CHECK(rand <= 300'000);
        std::this_thread::sleep_for(std::chrono::nanoseconds(time_dist(rng) / 2));
    }
}

namespace {
    struct test_broadcaster {
        using await_type = ldb::null_awaiter;
        ldb::lv::linda_tuple expected;
    };
    [[nodiscard]] ldb::null_awaiter
    broadcast_insert(test_broadcaster bcast, const lv::linda_tuple& value) {
        CHECK(bcast.expected == value);
        return {};
    }
    [[nodiscard]] ldb::null_awaiter
    broadcast_delete(test_broadcaster bcast, const lv::linda_tuple& value) {
        CHECK(bcast.expected == value);
        return {};
    }
    static_assert(ldb::broadcaster<test_broadcaster>);
}

TEST_CASE("broadcaster is notified when inserting with signaling") {
    ldb::store store;
    const auto tuple = ldb::lv::linda_tuple(1, 2, 3);
    store.set_broadcast(test_broadcaster{tuple});
    store.out(tuple);
}

TEST_CASE("serial insert,insert,remove,insert,remove runs") {
    static std::uniform_int_distribution<int> val_dist(100'000, 300'000);
    static std::mt19937_64 rng(std::random_device{}());
    ldb::store store;
    int rand{};
    auto query = ldb::query_tuple("asd", ldb::ref(&rand));

    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    store.out(lv::linda_tuple("asd", val_dist(rng)));
    std::ignore = store.in(query);
    auto ret = store.in(query);
    CHECK(ret[0] == lv::linda_value("asd"));
    CHECK(rand >= 100'000);
    CHECK(rand <= 300'000);
}

TEST_CASE("serial reads/writes proceeds",
          "[.long]") {
    static std::normal_distribution<double> key_dist(0, 100'000);
    static std::uniform_int_distribution<int> val_dist(0, 1000);
    static std::mt19937_64 rng(std::random_device{}());
    constexpr const static auto repeat_count = 1'000'000;

    ldb::store store;
    for (int i = 0; i < repeat_count; ++i) {
        const auto val = lv::linda_tuple(static_cast<std::int32_t>(key_dist(rng)), val_dist(rng));
        store.out(val);

        if (rng() % 2) {
            const auto key = static_cast<std::int32_t>(key_dist(rng));

            int read_val{};
            const auto query = ldb::query_tuple(static_cast<const std::int32_t>(key), ldb::ref(&read_val));
            const auto ret = store.inp(query);
            if (ret) {
                CHECK((*ret)[0] == lv::linda_value(key));
                CHECK(read_val >= val_dist.min());
                CHECK(read_val <= val_dist.max());
            }
        }
    }
}

TEST_CASE("store removes correct element for query") {
    ldb::store store;
    std::latch start(2);

    lv::linda_tuple tuple("asd", 2, "dsa");
    std::jthread const writer([&tuple, &store, &start] {
        start.arrive_and_wait();
        store.out(lv::linda_tuple("asd", 3, "dsa"));
        store.out(lv::linda_tuple("asd", 4, "dsa"));
        store.out(tuple); // 2
    });

    std::string data;
    start.arrive_and_wait();
    auto res = store.in(ldb::query_tuple("asd", 2, ldb::ref(&data)));
    CHECK(res == tuple);
}

TEST_CASE("store retrieves correct element for query") {
    ldb::store store;
    std::latch start(2);

    lv::linda_tuple tuple("asd", 2, "dsa");
    std::jthread const writer([&tuple, &store, &start] {
        start.arrive_and_wait();
        store.out(lv::linda_tuple("asd", 3, "dsa"));
        store.out(lv::linda_tuple("asd", 4, "dsa"));
        store.out(tuple); // 2
    });

    std::string data;
    start.arrive_and_wait();
    auto res = store.rd(ldb::query_tuple("asd", 2, ldb::ref(&data)));
    CHECK(res == tuple);
}

TEST_CASE("parallel reads/writes do not deadlock",
          "[.long]") {
    static std::uniform_int_distribution<unsigned> time_dist(500'000U, 1'000'000U);
    static std::normal_distribution<double> key_dist(0, 10'000);
    static std::uniform_int_distribution<int> val_dist(0, 1000);
    static std::mt19937_64 rng(std::random_device{}());
    constexpr const static auto repeat_count = 10'000;
    ldb::store store;
    std::mutex catch_guard{};

    auto adder = [&store](std::string_view name) {
        std::ignore = name;
        for (int i = 0; i < repeat_count; ++i) {
            const auto val = lv::linda_tuple(static_cast<std::int32_t>(key_dist(rng)),
                                             val_dist(rng));
            std::this_thread::sleep_for(std::chrono::nanoseconds(time_dist(rng)));
            store.out(val);
        }
    };
    auto gatherer = [&catch_guard, &store](std::string_view name) {
        std::ignore = name;
        for (int i = 0; i < repeat_count; ++i) {
            int rand{};
            const auto key = static_cast<std::int32_t>(key_dist(rng));
            const auto query = ldb::query_tuple(static_cast<const std::int32_t>(key),
                                                ldb::ref(&rand));
            std::this_thread::sleep_for(std::chrono::nanoseconds(time_dist(rng)) * 1.5);
            const auto ret = store.inp(query);
            if (!ret) continue;

            // Catch2 seems to break itself?
            // nothing else is shared at this point, so I don't **think** this is LindaDB?
            const std::scoped_lock lck(catch_guard);
            CHECK((*ret)[0] == lv::linda_value(key));
            CHECK(rand >= val_dist.min());
            CHECK(rand <= val_dist.max());
        }
    };

    const std::array thread_owner{
           std::jthread(adder, "adder1"),
           std::jthread(adder, "adder2"),
           std::jthread(adder, "adder3"),
           std::jthread(gatherer, "gatherer1"),
           std::jthread(gatherer, "gatherer2"),
           std::jthread(gatherer, "gatherer3"),
    };
}
