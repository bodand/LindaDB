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
 * Originally created: 2024-04-04.
 *
 * src/LindaRT/public/lrt/balancer/round_robin_balancer --
 *   
 */
#ifndef LINDADB_ROUND_ROBIN_BALANCER_HXX
#define LINDADB_ROUND_ROBIN_BALANCER_HXX

#include <atomic>
#include <random>
#include <utility>

#include <ldb/lv/linda_tuple.hxx>
#include <lrt/balancer/balancer_if.hxx>

namespace lrt {
    struct round_robin_balancer final {
        explicit round_robin_balancer(int comm_size) : _max(comm_size) { }

        round_robin_balancer(const round_robin_balancer& cp) noexcept : _max(cp._max) {
            LDBT_ZONE_A;
            _value.store(cp._value.load());
        }
        round_robin_balancer&
        operator=(const round_robin_balancer& cp) noexcept {
            LDBT_ZONE_A;
            _max = cp._max;
            _value.store(cp._value.load());
            return *this;
        }

        int
        send_to_rank(const ldb::lv::linda_tuple& /*ignored*/) {
            LDBT_ZONE_A;
            auto value = _value.load(std::memory_order_acquire);
            for (;;) {
                const auto next_candidate_value = value + 1;
                const auto next_value = next_candidate_value < _max ? next_candidate_value : 1;
                if (_value.compare_exchange_strong(value,
                                                   next_value,
                                                   std::memory_order_release,
                                                   std::memory_order_acquire))
                    return value;
            }
        }

    private:
        std::atomic<int> _value{1};
        int _max;
    };
}

#endif
