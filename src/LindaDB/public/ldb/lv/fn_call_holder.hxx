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
 * Originally created: 2024-02-22.
 *
 * src/LindaDB/public/ldb/lv/fn_call_holder --
 *   
 */
#ifndef LINDADB_FN_CALL_HOLDER_HXX
#define LINDADB_FN_CALL_HOLDER_HXX

#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace ldb::lv {
    struct linda_tuple;

    struct fn_call_holder final {
        fn_call_holder(std::string fn_name,
                       std::unique_ptr<linda_tuple>&& tuple);

        fn_call_holder(const fn_call_holder& cp);
        fn_call_holder&
        operator=(const fn_call_holder& cp);

        fn_call_holder(fn_call_holder&& mv) noexcept;
        fn_call_holder&
        operator=(fn_call_holder&& mv) noexcept;

        ~fn_call_holder();

        [[nodiscard]] const std::string&
        fn_name() const { return _fn_name; }

        [[nodiscard]] const linda_tuple&
        args() const { return *_args; }

        /**
         * \brief Executes the stored function call and injects its result into a tuple.
         *
         * \remarks
         * Executes the function from the current executable by its name (as stored), by
         * using the parameters in the stored tuple. The return tuple is created by taking
         * \c after_prefix elements from the \c elements tuple, then the result of the
         * function call, then the remaining elements.
         */
        lv::linda_tuple
        execute(int after_prefix, const linda_tuple& elements);

    private:
        friend struct ::std::hash<ldb::lv::fn_call_holder>;

        std::string _fn_name;
        std::unique_ptr<linda_tuple> _args;

        friend std::ostream&
        operator<<(std::ostream& os, const fn_call_holder& holder) {
            return os << "[fn call object: " << holder._fn_name << "]";
        }

        friend auto
        operator<=>(const fn_call_holder& rhs,
                    const fn_call_holder& lhs) { return rhs._fn_name <=> lhs._fn_name; }
        friend auto
        operator==(const fn_call_holder& rhs,
                   const fn_call_holder& lhs) { return rhs._fn_name == lhs._fn_name; }
    };
}

namespace std {
    template<>
    struct hash<ldb::lv::fn_call_holder> {
        std::size_t
        operator()(const ldb::lv::fn_call_holder& holder) const noexcept {
            return hash<std::string_view>{}(holder._fn_name);
        }
    };
}

#endif
