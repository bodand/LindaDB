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
 * Originally created: 2024-01-10.
 *
 * src/LindaDB/public/ldb/bcast/broadcaster --
 *   Interface requirements of a broadcaster implementation.
 */
#ifndef LINDADB_BROADCASTER_HXX
#define LINDADB_BROADCASTER_HXX

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <ldb/lv/linda_tuple.hxx>

namespace ldb {
    template<class Awaitable, class R>
    concept await_if = requires(Awaitable awaitable) {
        { await(awaitable) } -> std::same_as<R>;
    };

    template<class Context>
    struct broadcast_msg {
        int from;
        int tag;
        std::vector<std::byte> buffer;
        Context context;
    };

    template<class Broadcast,
             class RTerminate,
             class REval,
             class RInsert,
             class RDelete,
             class TContext>
    concept broadcast_if = requires(Broadcast bcast) {
        { broadcast_recv(bcast) } -> std::same_as<std::vector<broadcast_msg<TContext>>>;
        { broadcast_terminate(bcast) } -> await_if<RTerminate>;
        { send_eval(bcast, int{}, lv::linda_tuple{}) } -> await_if<REval>;
        { broadcast_insert(bcast, lv::linda_tuple{}) } -> await_if<RInsert>;
        { broadcast_delete(bcast, lv::linda_tuple{}) } -> await_if<RDelete>;
    };
}

#endif
