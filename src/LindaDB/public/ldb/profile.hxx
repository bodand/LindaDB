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
 * src/LindaDB/public/ldb/profile --
 *   
 */
#ifndef LINDADB_PROFILE_HXX
#define LINDADB_PROFILE_HXX

#ifdef LINDA_DB_PROFILING
#  include <type_traits>

#  include <tracy/Tracy.hpp>
#endif

#ifdef LINDA_DB_PROFILING
#  define LDBT_ZONE_A ZoneScoped
#  define LDBT_ZONE(...) ZoneScopedN(__VA_ARGS__)
#else
#  define LDBT_ZONE_A "empty"
#  define LDBT_ZONE(...) "empty"
#endif

#ifdef LINDA_DB_PROFILING
#  define LDBT_ALLOC(...) TracyAlloc(__VA_ARGS__)
#  define LDBT_FREE(...) TracyFree(__VA_ARGS__)
#else
#  define LDBT_ALLOC(...) "empty"
#  define LDBT_FREE(...) "empty"
#endif

#ifdef LINDA_DB_PROFILING
#  define LDBT_CV(name) std::condition_variable_any name

#  define LDBT_MUTEX_EX(type, name) TracyLockable(type, name)
#  define LDBT_LOCK_EX(name, mtx)                                                   \
      std::scoped_lock<std::remove_cvref_t<decltype(mtx)>> name(mtx); \
      LockMark(mtx)

#  define LDBT_SH_MUTEX_EX(type, name) TracySharedLockable(type, name)
#  define LDBT_UQ_LOCK_EX(name, mtx)                                  \
      std::unique_lock<std::remove_cvref_t<decltype(mtx)>> name(mtx); \
      LockMark(mtx)
#  define LDBT_SH_LOCK_EX(name, mtx)                                  \
      std::shared_lock<std::remove_cvref_t<decltype(mtx)>> name(mtx); \
      LockMark(mtx)
#else
#  define LDBT_CV(name) std::condition_variable_any name

#  define LDBT_MUTEX_EX(type, name) type name
#  define LDBT_LOCK_EX(name, mtx) std::scoped_lock<std::remove_cvref_t<decltype(mtx)>> name(mtx)

#  define LDBT_SH_MUTEX_EX(type, name) type name
#  define LDBT_UQ_LOCK_EX(name, mtx) std::unique_lock<std::remove_cvref_t<decltype(mtx)>> name(mtx)
#  define LDBT_SH_LOCK_EX(name, mtx) std::shared_lock<std::remove_cvref_t<decltype(mtx)>> name(mtx)
#endif

#define LDBT_MUTEX(name) LDBT_MUTEX_EX(std::mutex, name)
#define LDBT_SH_MUTEX(name) LDBT_SH_MUTEX_EX(std::shared_mutex, name)
#define LDBT_LOCK(mtx) LDBT_LOCK_EX(lck, mtx)
#define LDBT_UQ_LOCK(mtx) LDBT_UQ_LOCK_EX(lck, mtx)
#define LDBT_SH_LOCK(mtx) LDBT_SH_LOCK_EX(lck, mtx)


#endif
