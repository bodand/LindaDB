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
 * Originally created: 2024-03-05.
 *
 * demo/primes/primes --
 *   A Linda implementation of the sieve of Eratosthenes
 */

#include <algorithm>
#include <charconv>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <ranges>
#include <string>

#include <lrt/linda.hxx>

std::int64_t
get_random(int min = 0, int max = 99) {
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<std::int64_t> dist(min, max);
    return dist(rng);
}

int
func(std::string a,
     std::string b,
     std::string c) {
    int width;
    int height;
    rd("W", ldb::ref(&width));
    rd("H", ldb::ref(&height));

    for (auto i : std::views::iota(0, width)) {
        for (auto j : std::views::iota(0, height)) {
            if (!inp("<", i, j)) continue;
            int x = i;
            int y = j;

            std::int64_t value = 0;

            for (int k = 0; k < width; ++k) {
                std::int64_t value_a;
                std::int64_t value_b;

                rd(a, x, k, ldb::ref(&value_a));
                rd(b, k, y, ldb::ref(&value_b));

                value += value_a * value_b;
            }

            out(c,
                x,
                y,
                value);
        }
    }

    return 0;
}

int
real_main(int, char**) {
    const auto mx_width = 100;
    const auto mx_height = 100;

    out("W", mx_width);
    out("H", mx_height);

    const auto start = std::chrono::high_resolution_clock::now();
    for (auto i : std::views::iota(0, mx_width)) {
        for (auto j : std::views::iota(0, mx_height)) {
            auto val = get_random();
            out("A", i, j, val);
        }
    }

    for (auto i : std::views::iota(0, mx_height)) {
        for (auto j : std::views::iota(0, mx_width)) {
            auto val = get_random();
            out("B", i, j, val);
        }
    }

    for (auto i : std::views::iota(0, mx_width)) {
        for (auto j : std::views::iota(0, mx_height)) {
            out("<", i, j);
        }
    }

    std::cout << "start\n";
    const auto end = std::chrono::high_resolution_clock::now();

    for (int i : std::views::iota(1, lrt::this_runtime().world_size())) {
        eval("computed", (func) ("A", "B", "C"));
    }
    in("computed", 0);

    for (auto i : std::views::iota(0, mx_width)) {
        for (auto j : std::views::iota(0, mx_height)) {
            [[maybe_unused]] std::int64_t val;
            rd("C", i, j, ldb::ref(&val));
            std::ignore = val;
        }
    }

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "\n";

    return 0;
}
