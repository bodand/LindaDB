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
 * Originally created: 2024-03-10.
 *
 * demo/primes/primes --
 *   A pure C++ serial implementation of the primes example code meant as a baseline.
 */

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <ranges>
#include <vector>


template<std::random_access_iterator It,
         std::sentinel_for<It> S>
It
next_max_until(It current,
               S end,
               std::iter_difference_t<It> by = 1) {
    return current + std::min(by, std::distance(current, end));
}

template<std::random_access_iterator It,
         std::sentinel_for<It> S>
void
advance_max_until(It& current,
                  S end,
                  std::iter_difference_t<It> by = 1) {
    current = next_max_until(current, end, by);
}

int
main(int argc, char** argv) {
    constexpr static auto checked_range_start = 2;
    auto checked_range_end = 1000;

    if (argc > 1) std::from_chars(argv[1], argv[1] + std::strlen(argv[1]), checked_range_end);

    std::vector<int> values(checked_range_end - checked_range_start);
    std::iota(values.begin(), values.end(), checked_range_start);

    const auto value_to_index = [](int x) noexcept {
        return static_cast<std::size_t>(x) - checked_range_start;
    };

    std::ranges::for_each(values, [&value_to_index, &values](int val) {
        if (val == 0) return;
        auto sqr_idx = value_to_index(val * val);
        for (auto begin = next_max_until(values.begin(), values.end(), sqr_idx);
             begin != values.end();
             advance_max_until(begin, values.end(), val)) {
            *begin = 0;
        }
    });

    std::ranges::copy(values | std::views::filter(std::bind_front(std::not_equal_to{}, 0)),
                      std::ostream_iterator<int>(std::cout, " "));
}
