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
 * Originally created: 2024-03-02.
 *
 * src/LindaRT/public/lrt/balancer/uniform_random_balancer --
 *   
 */
#ifndef LINDADB_UNIFORM_RANDOM_BALANCER_HXX
#define LINDADB_UNIFORM_RANDOM_BALANCER_HXX

#include <random>
#include <utility>

#include <ldb/lv/linda_tuple.hxx>
#include <lrt/balancer/balancer_if.hxx>

namespace lrt {
    struct uniform_random_balancer final {
        uniform_random_balancer(int comm_size) : _distribution(1, std::max(1, comm_size - 1)) {
            LDBT_ZONE_A;
        }

        int
        send_to_rank(const ldb::lv::linda_tuple& /*ignored*/) {
            LDBT_ZONE_A;
            return _distribution(_rng);
        }

    private:
        std::mt19937_64 _rng{std::random_device{}()};
        std::uniform_int_distribution<int> _distribution;
    };
}

#endif
