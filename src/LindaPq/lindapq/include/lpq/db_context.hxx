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
 * Originally created: 2024-10-10.
 *
 * src/LindaPq/lindapq/include/db_context --
 *   
 */
#ifndef DB_CONTEXT_HXX
#define DB_CONTEXT_HXX

#include <atomic>
#include <memory>
#include <span>
#include <string>
#include <optional>
#include <string_view>

namespace lpq {
    struct db_notify_awaiter;
}
namespace ldb::lv {
    struct linda_tuple;
}

namespace lpq {
    struct db_context {
        using value_type = int;

        db_context();

        std::string
        prepare(std::string_view sql);

        std::optional<ldb::lv::linda_tuple>
        exec_prepared(const std::string& stmt, std::span<const char*> params) const;

        void
        deallocate(const std::string& stmt) const;

        void*
        native_handle() const noexcept { return _conn.get(); }

        db_notify_awaiter
        listen(std::string_view channel);

    private:
        std::string
        next_name();

        struct conn_freer {
            void
            operator()(void* conn) const noexcept;
        };
        using conn_type = std::unique_ptr<void, conn_freer>;

        std::atomic<unsigned> _stmt_namer{};
        conn_type _conn;
    };
}

#endif
