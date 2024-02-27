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
 * Originally created: 2024-02-27.
 *
 * src/LindaDB/public/ldb/lv/fn_call_builder --
 *   An type used to build a function call object from a given linda tuple.
 */
#ifndef LINDADB_FN_CALL_BUILDER_HXX
#define LINDADB_FN_CALL_BUILDER_HXX

#include <memory>
#include <string_view>

#include <ldb/lv/linda_value.hxx>

namespace ldb::lv {
    struct fn_call_builder {
        fn_call_builder(const fn_call_builder& cp) = delete;
        fn_call_builder&
        operator=(const fn_call_builder& cp) = delete;

        fn_call_builder(fn_call_builder&& mv) noexcept = delete;
        fn_call_builder&
        operator=(fn_call_builder&& mv) noexcept = delete;

        [[nodiscard]] virtual std::unique_ptr<fn_call_builder>
        add_arg(const linda_value&) = 0;

        [[nodiscard]] virtual linda_value
        finalize(std::string_view function_name) const = 0;

        virtual ~fn_call_builder() = default;

    protected:
        fn_call_builder() noexcept = default;
    };

    std::unique_ptr<fn_call_builder>
    get_call_builder();
}

#endif
