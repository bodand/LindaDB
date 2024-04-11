/* LindaDB project
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
MIMALLOC_NEW_DELETE_H

#include <ldb/common.hxx>
#include <ldb/profile.hxx>

#ifdef LINDA_DB_USE_MIMALLOC

#  define LDB_MALLOC mi_new
#  define LDB_MALLOC_NT mi_new_nothrow
#  define LDB_FREE mi_free
#  define LDB_FREE_SIZE mi_free_size
#  define LDB_AMALLOC mi_new_aligned
#  define LDB_AMALLOC_NT mi_new_aligned_nothrow
#  define LDB_AFREE mi_free_aligned
#  define LDB_AFREE_SIZE mi_free_size_aligned

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
#  if defined(__cplusplus)
#    include <new>

#    include <mimalloc.h>

#    if defined(_MSC_VER) && defined(_Ret_notnull_) && defined(_Post_writable_byte_size_)
// stay consistent with VCRT definitions
#      define mi_decl_new(n) mi_decl_nodiscard mi_decl_restrict _Ret_notnull_ _Post_writable_byte_size_(n)
#      define mi_decl_new_nothrow(n) mi_decl_nodiscard mi_decl_restrict _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(n)
#    else
#      define mi_decl_new(n) mi_decl_nodiscard mi_decl_restrict
#      define mi_decl_new_nothrow(n) mi_decl_nodiscard mi_decl_restrict
#    endif

#    ifdef _MSC_VER
#      pragma warning(push)
#      pragma warning(disable: 4559)
#    endif

#  endif

#else
#  include <cstdlib>

#  define LDB_MALLOC std::malloc
#  define LDB_MALLOC_NT std::malloc
#  ifdef _MSC_VER
#    define LDB_AMALLOC _aligned_malloc
#    define LDB_AMALLOC_NT _aligned_malloc
#    define LDB_AFREE(p, al) _aligned_free(p)
#    define LDB_AFREE_SIZE(p, n, al) _aligned_free(p)
#  else
#    define LDB_AMALLOC(sz, al) std::aligned_alloc(al, sz)
#    define LDB_AMALLOC_NT(sz, al) std::aligned_alloc(al, sz)
#    define LDB_AFREE(p, al) std::free(p)
#    define LDB_AFREE_SIZE(p, n, al) std::free(p)
#  endif
#  define LDB_FREE std::free
#  define LDB_FREE_SIZE(p, n) std::free(p)

#  define mi_decl_new(n)
#  define mi_decl_new_nothrow(n)
#endif


void
operator delete(void* p) noexcept {
    LDBT_FREE(p);
    LDB_FREE(p);
}
void
operator delete[](void* p) noexcept {
    LDBT_FREE(p);
    LDB_FREE(p);
}

void
operator delete(void* p, const std::nothrow_t& tag) noexcept {
    LDBT_FREE(p);
    (void) tag;
    LDB_FREE(p);
}
void
operator delete[](void* p, const std::nothrow_t& tag) noexcept {
    LDBT_FREE(p);
    (void) tag;
    LDB_FREE(p);
}

mi_decl_new(n) void*
operator new(std::size_t n) noexcept(false) {
    void* res = LDB_MALLOC(n);
    LDBT_ALLOC(res, n);
    return res;
}
mi_decl_new(n) void*
operator new[](std::size_t n) noexcept(false) {
    void* res = LDB_MALLOC(n);
    LDBT_ALLOC(res, n);
    return res;
}

mi_decl_new_nothrow(n) void*
operator new(std::size_t n, const std::nothrow_t& tag) noexcept {
    (void) (tag);
    void* res = LDB_MALLOC_NT(n);
    LDBT_ALLOC(res, n);
    return res;
}
mi_decl_new_nothrow(n) void*
operator new[](std::size_t n, const std::nothrow_t& tag) noexcept {
    (void) (tag);
    void* res = LDB_MALLOC_NT(n);
    LDBT_ALLOC(res, n);
    return res;
}

#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void
operator delete(void* p, std::size_t n) noexcept {
    LDBT_FREE(p);
    LDB_FREE_SIZE(p, n);
}
void
operator delete[](void* p, std::size_t n) noexcept {
    LDBT_FREE(p);
    LDB_FREE_SIZE(p, n);
}
#endif

#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void
operator delete(void* p, std::align_val_t al) noexcept {
    LDBT_FREE(p);
    LDB_AFREE(p, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::align_val_t al) noexcept {
    LDBT_FREE(p);
    LDB_AFREE(p, static_cast<size_t>(al));
}
void
operator delete(void* p, std::size_t n, std::align_val_t al) noexcept {
    LDBT_FREE(p);
    LDB_AFREE_SIZE(p, n, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept {
    LDBT_FREE(p);
    LDB_AFREE_SIZE(p, n, static_cast<size_t>(al));
}
void
operator delete(void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    LDBT_FREE(p);
    (void) tag;
    LDB_AFREE(p, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    LDBT_FREE(p);
    (void) tag;
    LDB_AFREE(p, static_cast<size_t>(al));
}

void*
operator new(std::size_t n, std::align_val_t al) noexcept(false) {
    void* res = LDB_AMALLOC(n, static_cast<size_t>(al));
    LDBT_ALLOC(res, n);
    return res;
}
void*
operator new[](std::size_t n, std::align_val_t al) noexcept(false) {
    void* res = LDB_AMALLOC(n, static_cast<size_t>(al));
    LDBT_ALLOC(res, n);
    return res;
}
void*
operator new(std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    void* res = LDB_AMALLOC_NT(n, static_cast<size_t>(al));
    LDBT_ALLOC(res, n);
    return res;
}
void*
operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    void* res = LDB_AMALLOC_NT(n, static_cast<size_t>(al));
    LDBT_ALLOC(res, n);
    return res;
}
#endif

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#endif
