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
 * Originally created: 2023-11-29.
 *
 * src/LindaDB/common --
 *   Common utils for LindaDB sources.
 */
#ifndef LDB_COMMON_HXX
#define LDB_COMMON_HXX

#ifdef __GNUC__
#  define LDB_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#  define LDB_UNREACHABLE __assume(0)
#else // ¯\_(ツ)_/¯
#  define LDB_UNREACHABLE ((void) 0)
#endif

#if __cplusplus > 202002L
#  define LDB_CONSTEXPR23 constexpr
#else
#  define LDB_CONSTEXPR23
#endif

#if LDB_HAVE_STACKTRACE
#  include <stacktrace>
#  define std_stacktrace std::stacktrace
#else
#  include <ostream>
struct std_stacktrace {
    static std_stacktrace
    current() { return {}; }

private:
    friend std::ostream&
    operator<<(std::ostream& os, std_stacktrace /*unused*/) {
        return os << "<<STACKTRACE NOT SUPPORTED WHEN COMPILED IN C++20 COMPATIBILITY MODE>>";
    }
};
#endif

#include <source_location>
#include <string_view>

#define LDB_STR_I(x) #x
#define LDB_STR(x) LDB_STR_I(x)
#define assert_that(cond, ...) ::ldb::assert_that_impl(static_cast<bool>(cond), LDB_STR(cond) __VA_OPT__(, ) __VA_ARGS__)

namespace ldb {
    void
    assert_that_impl(bool cond,
                     std::string_view cond_stringified,
                     std::string_view message = "",
                     std::source_location source_loc = std::source_location::current());
}

#endif
