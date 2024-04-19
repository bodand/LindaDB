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

long long
get_random(int min = 0, int max = 99) {
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<long long> dist(min, max);
    return dist(rng);
}

int
func(std::string a,
     std::string b,
     std::string c) {
    int width;
    rd("W", ldb::ref(&width));

    //    for (auto i : std::views::iota(0, width)) {
    //        for (auto j : std::views::iota(0, width)) {
    //            out("C", i, j, 0LL);
    //        }
    //    }
    int x, y;
    while (inp("<", ldb::ref(&x), ldb::ref(&y))) {
        long long value = 0;

        for (int i = 0; i < width; ++i) {
            long long value_a;
            long long value_b;

            rd(a, x, i, ldb::ref(&value_a));
            rd(b, i, y, ldb::ref(&value_b));

            value += value_a * value_b;
        }

        out(c,
            x,
            y,
            value);
    }

    return 0;
}

int
real_main(int, char**) {
    const auto mx_width = 100;
    const auto mx_height = 100;

    out("W", mx_width);
    out("H", mx_height);

    for (auto i : std::views::iota(0, mx_width)) {
        for (auto j : std::views::iota(0, mx_height)) {
            auto val = get_random();
            out("A", i, j, val);
            std::cout << std::setw(2) << val << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "\n\n";

    for (auto i : std::views::iota(0, mx_height)) {
        for (auto j : std::views::iota(0, mx_width)) {
            auto val = get_random();
            out("B", i, j, val);
            std::cout << std::setw(2) << val << ' ';
        }
        std::cout << '\n';
    }

    for (auto i : std::views::iota(0, mx_width)) {
        for (auto j : std::views::iota(0, mx_height)) {
            out("<", i, j);
        }
    }

    std::ofstream("_a.log", std::ios::app) << lrt::this_runtime().rank() << ": INITED\n";
    for (int i : std::views::iota(1, lrt::this_runtime().world_size())) {
        eval((func) ("A", "B", "C"));
    }

    std::ofstream fout("out.txt");
    std::cout << "\n\n";
    for (auto i : std::views::iota(0, mx_width)) {
        for (auto j : std::views::iota(0, mx_height)) {
            long long val;
            rd("C", i, j, ldb::ref(&val));
            fout << std::setw(7) << val << ' ';
        }
        fout << '\n';
    }
    std::ofstream("_a.log", std::ios::app) << lrt::this_runtime().rank() << ": DONE\n";

    return 0;
}
