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
 * src/LindaDB/public/profiler --
 *   File for handling the flipping of the profiler used in the DB.
 */
#ifndef LINDADB_PROFILER_HXX
#define LINDADB_PROFILER_HXX

#ifdef LINDA_DB_PROFILER
#  include <tracy/Tracy.hpp>
#endif

namespace ldb::prof {
    constexpr const static auto color_search = 0xF0F000;
    constexpr const static auto color_insert = 0xF0F000;
}

#ifdef __GNUC__
#  define LDB_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#  define LDB_UNREACHABLE __assume(0)
#else // ¯\_(ツ)_/¯
#  define LDB_UNREACHABLE ((void) 0)
#endif

// When using the profiler and in C++23 or higher
#if defined(LINDA_DB_PROFILER) && __cplusplus > 202002L
#  define LDB_CONSTEXPR23 constexpr
#else
#  define LDB_CONSTEXPR23
#endif

#ifdef ZoneScopedNC
#  define LDB_PROF_SCOPE_C ZoneScopedNC
#else
#  define LDB_PROF_SCOPE_C(...) ((void) 0)
#endif

#ifdef ZoneScopedN
#  define LDB_PROF_SCOPE ZoneScopedN
#else
#  define LDB_PROF_SCOPE(...) ((void) 0)
#endif

#ifdef TracyMessageL
#  define LDB_PROF_MSG TracyMessageL
#else
#  define LDB_PROF_MSG(...) ((void) 0)
#endif

#ifdef TracyAlloc
#  define LDB_PROF_ALLOC TracyAlloc
#else
#  define LDB_PROF_ALLOC(...) ((void) 0)
#endif

#ifdef TracyFree
#  define LDB_PROF_DEALLOC TracyFree
#else
#  define LDB_PROF_DEALLOC(...) ((void) 0)
#endif

#endif
