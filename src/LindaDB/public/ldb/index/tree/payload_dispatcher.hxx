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
 * Originally created: 2023-10-14.
 *
 * src/LindaDB/public/ldb/index/tree/payload_dispatcher --
 *   A compile time dispatcher for figuring out and choosing the best payload-type
 *   for a given key-value pair.
 */
#ifndef LINDADB_PAYLOAD_DISPATCHER_HXX
#define LINDADB_PAYLOAD_DISPATCHER_HXX

#include <cstdlib>
#include <memory>

#include <ldb/index/tree/payload/chime_payload.hxx>
#include <ldb/index/tree/payload/scalar_payload.hxx>
#include <ldb/index/tree/payload/vector_payload.hxx>

namespace ldb::index::tree {
    consteval std::size_t
    cluster_for_minimized_overhead_effect(std::size_t payload_size,
                                          std::size_t node_overhead,
                                          const std::size_t clustering_req) {
        constexpr const auto accept_level_value = .979999998L;
        if (clustering_req != 0) return clustering_req;

        const auto total_payload = [payload_size](std::size_t factor) {
            return static_cast<long double>(factor * payload_size);
        };
        const auto total_size = [&total_payload, node_overhead](std::size_t factor) {
            return total_payload(factor) + static_cast<long double>(node_overhead);
        };

        std::size_t factor = 1;
        while (total_payload(factor) / total_size(factor) < accept_level_value) {
            ++factor;
        }
        return factor;
    }

    template<class K, class V, std::size_t Clustering = 0>
    struct payload_dispatcher {
        constexpr const static std::size_t overhead_size = sizeof(std::unique_ptr<int>) * 2
                                                           + sizeof(std::unique_ptr<int>*);
        using type = payloads::chime_payload<
               K,
               V,
               cluster_for_minimized_overhead_effect(sizeof(K) + sizeof(std::vector<V>), overhead_size, Clustering)>;
    };

    template<class K, class V>
    struct payload_dispatcher<K, V, 1> {
        using type = payloads::scalar_payload<K, V>;
    };
}
#endif
