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
    out("m_lock");
    out("m", 2);
}

int
eliminate_multiples(int p, int n) {
    std::ofstream("_primes.log", std::ios::app) << "p: " << p << "\n";

    {
        std::ofstream os("_primes.log", std::ios::app);
        os << "RANK: " << lrt::this_runtime().rank() << " INDEX DUMP:\n";
        lrt::this_store().dump_indices(os);
    }


    int current_num;
    in("m", ldb::ref(&current_num));
    out("m", current_num + 1);
    std::ofstream("_primes.log", std::ios::app) << "num: " << current_num << "\n";
    while (current_num * current_num < n) {
        if (rdp("vec", current_num)) {
            int square = current_num * current_num;
            while (square < n) {
                inp("vec", square);
                square = square + current_num;
            }
        }
        in("m", ldb::ref(&current_num));
        out("m", current_num + 1);
    }

    out("done", p);
    std::ofstream("_primes.log", std::ios::app) << "p: " << p << "--done: " << "\n";

    return 0;
}

int
real_main() {
    constexpr static auto checked_range_end = 1000;
    initialize_values(checked_range_end);
    std::ofstream("_primes.log", std::ios::app) << "Initialized...\n";
    {
        std::ofstream os("_primes.log", std::ios::app);
        lrt::this_store().dump_indices(os);
    }

    std::ranges::for_each(std::views::iota(0, lrt::this_runtime().world_size() - 1),
                          [](int i) {
                              eval((eliminate_multiples) (i, checked_range_end));
                          });

    int proc = 1;
    while (proc < lrt::this_runtime().world_size() - 1) {
        in("done", proc);
        ++proc;
    }

    std::cout << "Primes in [1.." << checked_range_end << "]:\n";
    int i;
    while (inp("vec", ldb::ref(&i))) std::cout << " " << i;

    return 0;
}
