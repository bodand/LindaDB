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
#include <fstream>

#include <lrt/linda.hxx>

void
initialize_values(int n) {
    std::ranges::for_each(std::views::iota(2, n), [](int i) {
        out("vec", i);
    });
    out("m", 2);
}

constexpr static auto checked_range_end = 300;

int
eliminate_multiples(int p) {
    std::ofstream("_primes.log", std::ios::app) << "p: " << p << "\n";

    int current_num;
    in("m", ldb::ref(&current_num));

    out("m", current_num + 1);
    std::ofstream("_primes.log", std::ios::app) << "num: " << current_num << "\n";

    while (current_num * current_num < checked_range_end) {
        auto vec = rdp("vec", current_num);
        std::ofstream("_primes.log", std::ios::app) << "(vec, " << current_num << ")\n";
        if (vec) {
            int square = current_num * current_num;
            while (square < checked_range_end) {
                inp("vec", square);
                square = square + current_num;
            }
        }
        in("m", ldb::ref(&current_num));
        std::ofstream("_primes.log", std::ios::app) << "num: " << current_num << "\n";
        out("m", current_num + 1);
    }

    std::ofstream("_primes.log", std::ios::app) << "p: " << p << "--done: "
                                                << "\n";

    return p;
}

int
real_main() {
    initialize_values(checked_range_end);
    std::ofstream("_primes.log", std::ios::app) << "Initialized...\n";

    std::ranges::for_each(std::views::iota(0, lrt::this_runtime().world_size() - 1),
                          [](int i) {
                              eval("done", (eliminate_multiples) (i));
                          });
    std::ranges::for_each(std::views::iota(0, lrt::this_runtime().world_size() - 1),
                          [](int i) {
                              in("done", i);
                          });

    std::cout << "Primes in [1.." << checked_range_end << "]:\n";
    int i;
    while (inp("vec", ldb::ref(&i))) std::cout << " " << i;

    return 0;
}
