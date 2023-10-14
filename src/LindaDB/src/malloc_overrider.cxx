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
 * src/LindaDB/src/malloc_overrider --
 *   Source file to override standard C++ ::new and ::delete operations with the
 *   required mimalloc backend calls.
 */

// PATCHED mimalloc source for profiler integration
/* ----------------------------------------------------------------------------
Copyright (c) 2018-2020 Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#ifndef MIMALLOC_NEW_DELETE_H
#define MIMALLOC_NEW_DELETE_H

#include <ldb/profiler.hxx>

// ----------------------------------------------------------------------------
// This header provides convenient overrides for the new and
// delete operations in C++.
//
// This header should be included in only one source file!
//
// On Windows, or when linking dynamically with mimalloc, these
// can be more performant than the standard new-delete operations.
// See <https://en.cppreference.com/w/cpp/memory/new/operator_new>
// ---------------------------------------------------------------------------
#if defined(__cplusplus)
#  include <new>

#  include <mimalloc.h>

#  if defined(_MSC_VER) && defined(_Ret_notnull_) && defined(_Post_writable_byte_size_)
// stay consistent with VCRT definitions
#    define mi_decl_new(n) mi_decl_nodiscard mi_decl_restrict _Ret_notnull_ _Post_writable_byte_size_(n)
#    define mi_decl_new_nothrow(n) mi_decl_nodiscard mi_decl_restrict _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(n)
#  else
#    define mi_decl_new(n) mi_decl_nodiscard mi_decl_restrict
#    define mi_decl_new_nothrow(n) mi_decl_nodiscard mi_decl_restrict
#  endif

#  ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable: 4559)
#  endif

void
operator delete(void* p) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free(p);
}
void
operator delete[](void* p) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free(p);
}

void
operator delete(void* p, const std::nothrow_t& tag) noexcept {
    (void) tag;
    LDB_PROF_DEALLOC(p);
    mi_free(p);
}
void
operator delete[](void* p, const std::nothrow_t& tag) noexcept {
    (void) tag;
    LDB_PROF_DEALLOC(p);
    mi_free(p);
}

mi_decl_new(n) void*
operator new(std::size_t n) noexcept(false) {
    void* res = mi_new(n);
    LDB_PROF_ALLOC(res, n);
    return res;
}
mi_decl_new(n) void*
operator new[](std::size_t n) noexcept(false) {
    void* res = mi_new(n);
    LDB_PROF_ALLOC(res, n);
    return res;
}

mi_decl_new_nothrow(n) void*
operator new(std::size_t n, const std::nothrow_t& tag) noexcept {
    (void) (tag);
    void* res = mi_new_nothrow(n);
    LDB_PROF_ALLOC(res, n);
    return res;
}
mi_decl_new_nothrow(n) void*
operator new[](std::size_t n, const std::nothrow_t& tag) noexcept {
    (void) (tag);
    void* res = mi_new_nothrow(n);
    LDB_PROF_ALLOC(res, n);
    return res;
}

#  if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void
operator delete(void* p, std::size_t n) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free_size(p, n);
}
void
operator delete[](void* p, std::size_t n) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free_size(p, n);
}
#  endif

#  if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void
operator delete(void* p, std::align_val_t al) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free_aligned(p, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::align_val_t al) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free_aligned(p, static_cast<size_t>(al));
}
void
operator delete(void* p, std::size_t n, std::align_val_t al) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free_size_aligned(p, n, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept {
    LDB_PROF_DEALLOC(p);
    mi_free_size_aligned(p, n, static_cast<size_t>(al));
}
void
operator delete(void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    LDB_PROF_DEALLOC(p);
    mi_free_aligned(p, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    LDB_PROF_DEALLOC(p);
    mi_free_aligned(p, static_cast<size_t>(al));
}

void*
operator new(std::size_t n, std::align_val_t al) noexcept(false) {
    void* res = mi_new_aligned(n, static_cast<size_t>(al));
    LDB_PROF_ALLOC(res, n);
    return res;
}
void*
operator new[](std::size_t n, std::align_val_t al) noexcept(false) {
    void* res = mi_new_aligned(n, static_cast<size_t>(al));
    LDB_PROF_ALLOC(res, n);
    return res;
}
void*
operator new(std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    void* res = mi_new_aligned_nothrow(n, static_cast<size_t>(al));
    LDB_PROF_ALLOC(res, n);
    return res;
}
void*
operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    void* res = mi_new_aligned_nothrow(n, static_cast<size_t>(al));
    LDB_PROF_ALLOC(res, n);
    return res;
}
#  endif
#endif

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#endif // MIMALLOC_NEW_DELETE_H
