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
 * src/LindaDB/public/ldb/bcast/null_broadcast --
 *   A special broadcast implementation that does nothing.
 */
#ifndef LDB_YES_BROADCAST_HXX
#define LDB_YES_BROADCAST_HXX

#include <tuple>

#include <ldb/bcast/broadcaster.hxx>
#include <ldb/bcast/null_broadcast.hxx>
#include <ldb/lv/linda_tuple.hxx>

namespace ldb {
    struct yes_awaiter { };

    constexpr bool
    await(yes_awaiter) {
        return true;
    }

    template<class RTerminate,
             class REval,
             class TContext>
    struct yes_mock_broadcast {
        using context_type = bool;
    };

    template<class RTerminate,
             class REval,
             class TContext>
    yes_awaiter
    broadcast_insert(yes_mock_broadcast<RTerminate, REval, TContext>, const lv::linda_tuple& tuple) {
        std::ignore = tuple;
        return {};
    }

    template<class RTerminate,
             class REval,
             class TContext>
    yes_awaiter
    broadcast_delete(yes_mock_broadcast<RTerminate, REval, TContext>, const lv::linda_tuple& tuple) {
        std::ignore = tuple;
        return {};
    }

    template<class RTerminate,
             class REval,
             class TContext>
    std::vector<broadcast_msg<TContext>>
    broadcast_recv(yes_mock_broadcast<RTerminate, REval, TContext>) {
        return {};
    }

    template<class RTerminate,
             class REval,
             class TContext>
    null_awaiter<REval>
    send_eval(yes_mock_broadcast<RTerminate, REval, TContext>, int target, const lv::linda_tuple& tuple) {
        std::ignore = target;
        std::ignore = tuple;
        return null_awaiter<REval>{};
    }

    template<class RTerminate,
             class TContext>
    yes_awaiter
    send_eval(yes_mock_broadcast<RTerminate, bool, TContext>, int target, const lv::linda_tuple& tuple) {
        std::ignore = target;
        std::ignore = tuple;
        return {};
    }

    template<class RTerminate,
             class REval,
             class TContext>
    null_awaiter<RTerminate>
    broadcast_terminate(yes_mock_broadcast<RTerminate, REval, TContext>) {
        return null_awaiter<RTerminate>{};
    }

    template<class REval,
             class TContext>
    yes_awaiter
    broadcast_terminate(yes_mock_broadcast<bool, REval, TContext>) {
        return {};
    }
}

#endif
