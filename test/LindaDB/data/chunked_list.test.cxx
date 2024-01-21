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
 * test/LindaDB/data/chunked_list --
 *   Tests for the chunked list implementation.
 */

#include <tuple>
#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-inc-dec-in-conditions"

#include <compare>
#include <concepts>
#include <iterator>
#include <numeric>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>
#include <ldb/data/chunked_list.hxx>

static constexpr const auto gDouble_Chunk_Size = 32;
namespace ld = ldb::data;

template<class I>
concept LegacyIterator =
       requires(I i) {
           { *i } -> std::same_as<typename I::reference>;
           { ++i } -> std::same_as<I&>;
           // https://en.cppreference.com/w/cpp/named_req/Iterator/ says this is needed for
           // __LegacyIterator but in reality it doesn't seem like it from the actual description
           //           { *i++ } -> std::same_as<typename I::reference>;
       } && std::copyable<I>;

template<class I>
concept LegacyInputIterator =
       LegacyIterator<I>
       && std::equality_comparable<I>
       && requires(I i) {
              typename std::incrementable_traits<I>::difference_type;
              typename std::indirectly_readable_traits<I>::value_type;
              typename std::common_reference_t<std::iter_reference_t<I>&&,
                                               typename std::indirectly_readable_traits<I>::value_type&>;
              *i++;
              typename std::common_reference_t<decltype(*i++)&&,
                                               typename std::indirectly_readable_traits<I>::value_type&>;
              requires std::signed_integral<typename std::incrementable_traits<I>::difference_type>;
          };

template<class Iter>
concept LegacyForwardIterator =
       LegacyInputIterator<Iter>
       && std::constructible_from<Iter>
       && std::is_reference_v<std::iter_reference_t<Iter>>
       && std::same_as<std::remove_cvref_t<std::iter_reference_t<Iter>>, typename std::indirectly_readable_traits<Iter>::value_type>
       && requires(Iter it) {
              { it++ } -> std::convertible_to<const Iter&>;
              { *it++ } -> std::same_as<std::iter_reference_t<Iter>>;
          };

template<class I>
concept LegacyBidirectionalIterator =
       LegacyForwardIterator<I> && requires(I i) {
           { --i } -> std::same_as<I&>;
           { i-- } -> std::convertible_to<const I&>;
           { *i-- } -> std::same_as<std::iter_reference_t<I>>;
       };

using it_type = typename ld::chunked_list<int>::iterator;

TEST_CASE("chunked_list<...>::iterator conforms to LegacyIterator",
          "[chunked_list][iterator]") {
    STATIC_REQUIRE(std::destructible<it_type>);
    STATIC_REQUIRE(std::swappable<it_type>);
    STATIC_REQUIRE(std::copy_constructible<it_type>);
    STATIC_REQUIRE(std::is_copy_assignable_v<it_type>);
    STATIC_REQUIRE(LegacyIterator<it_type>);
}

TEST_CASE("chunked_list<...>::iterator conforms to LegacyInputIterator",
          "[chunked_list][iterator]") {
    STATIC_REQUIRE(LegacyInputIterator<it_type>);
    STATIC_REQUIRE(std::input_iterator<it_type>);
}

TEST_CASE("chunked_list<...>::iterator conforms to LegacyForwardIterator",
          "[chunked_list][iterator]") {
    STATIC_REQUIRE(LegacyForwardIterator<it_type>);
    STATIC_REQUIRE(std::forward_iterator<it_type>);
}

TEST_CASE("chunked_list<...>::iterator conforms to LegacyBidirectionalIterator",
          "[chunked_list][iterator]") {
    STATIC_REQUIRE(LegacyBidirectionalIterator<it_type>);
    STATIC_REQUIRE(std::bidirectional_iterator<it_type>);
}

TEST_CASE("chunked_list<...>::iterator conforms to three-way-comparable",
          "[chunked_list][iterator]") {
    STATIC_REQUIRE(std::three_way_comparable<it_type>);
}

TEST_CASE("default constructed chunked_list has empty size") {
    const ld::chunked_list<int> data;
    CHECK(data.size() == 0); // NOLINT
}

TEST_CASE("default constructed chunked_list is empty") {
    const ld::chunked_list<int> data;
    CHECK(data.empty());
}

TEST_CASE("chunked_list can be pushed_back to") {
    ld::chunked_list<int> data;
    data.push_back(2);
    CHECK_FALSE(data.empty());
}

TEST_CASE("chunked_list can be emplace_back to") {
    ld::chunked_list<int> data;
    data.emplace_back(2);
    CHECK_FALSE(data.empty());
}

TEST_CASE("chunked_list with one element has size 1") {
    ld::chunked_list<int> data;
    data.push_back(2);
    CHECK(data.size() == 1);
}

TEST_CASE("chunked_list has correct size after multiple push_backs") {
    ld::chunked_list<int> data;
    for (int i = 0; i < gDouble_Chunk_Size; ++i) {
        data.push_back(i);
    }
    CHECK(data.size() == 32U);
}

TEST_CASE("chunked_list has correct size after multiple emplace_backs") {
    ld::chunked_list<int> data;
    for (int i = 0; i < gDouble_Chunk_Size; ++i) {
        data.emplace_back(i);
    }
    CHECK(data.size() == 32U);
}

TEST_CASE("chunked_list is empty after clear") {
    ld::chunked_list<int> data;
    for (int i = 0; i < gDouble_Chunk_Size; ++i) {
        data.emplace_back(i);
    }
    CHECK(data.size() == 32U);
    data.clear();
    CHECK(data.empty());
}

TEST_CASE("empty chunked_list can be iterated over") {
    const ld::chunked_list<int> data;
    const auto res = std::accumulate(data.begin(), data.end(), 0, [](auto acc, auto elem) {
        std::ignore = elem;
        return acc + 1;
    });
    CHECK(res == 0);
}

TEST_CASE("single element chunked_list can be iterated over") {
    ld::chunked_list<int> data;
    data.push_back(1);
    for (int const i : data) {
        CHECK(i == 1);
    }
}

TEST_CASE("chunked_list can be iterated over") {
    ld::chunked_list<int> data;
    for (int i = 0; i < gDouble_Chunk_Size; ++i) {
        data.emplace_back(i + 1);
    }
    for (int const& it : data) {
        CHECK(it > 0);
    }
}

TEST_CASE("chunked_list can be linearly searched in even if there are holes") {
    ld::chunked_list<int> data;
    for (int i = 0; i < gDouble_Chunk_Size; ++i) {
        data.emplace_back(i + 1);
    }
    data.erase(std::next(std::begin(data), gDouble_Chunk_Size / 2));
    auto it = std::ranges::find(data, gDouble_Chunk_Size);
    CHECK(*it == gDouble_Chunk_Size);
}

TEST_CASE("chunked_list can be deleted from using an iterator") {
    ld::chunked_list<int> data;
    for (int i = 0; i < gDouble_Chunk_Size; ++i) {
        data.emplace_back(i);
    }
    data.erase(std::next(data.begin(), 3));
    for (int const& it : data) {
        CHECK(it != 3);
    }
}

// NOLINTNEXTLINE
TEST_CASE("chunked_list iterators are comparable") {
    ld::chunked_list<int> data;
    for (int i = 0; i < gDouble_Chunk_Size; ++i) {
        data.emplace_back(i + 1);
    }

    SECTION("iterators that are equal, compare equal") {
        CHECK(data.begin() == data.begin());
        CHECK(++data.begin() == ++data.begin());
        CHECK(--data.end() == --data.end());
        CHECK(data.end() == data.end());
    }

    SECTION("iterators that are equal, compare equal") {
        CHECK(data.begin() < ++data.begin());
        CHECK(++data.begin() > data.begin());
        CHECK(--data.end() < data.end());
        CHECK(data.end() > --data.end());
    }
}

#pragma clang diagnostic pop
