/* LindaDB project
 *
 * Originally created: 2023-10-14.
 *
 * src/LindaDB/src/malloc_overrider --
 *   Source file to override standard C++ ::new and ::delete operations with the
 *   required mimalloc backend calls.
 */

/* ----------------------------------------------------------------------------
Copyright (c) 2018-2020 Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#ifndef MIMALLOC_NEW_DELETE_H
#define MIMALLOC_NEW_DELETE_H
MIMALLOC_NEW_DELETE_H

#ifdef LINDA_DB_USE_MIMALLOC

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

void
operator delete(void* p) noexcept {
    mi_free(p);
}
void
operator delete[](void* p) noexcept {
    mi_free(p);
}

void
operator delete(void* p, const std::nothrow_t& tag) noexcept {
    (void) tag;
    mi_free(p);
}
void
operator delete[](void* p, const std::nothrow_t& tag) noexcept {
    (void) tag;
    mi_free(p);
}

mi_decl_new(n) void*
operator new(std::size_t n) noexcept(false) {
    void* res = mi_new(n);
    return res;
}
mi_decl_new(n) void*
operator new[](std::size_t n) noexcept(false) {
    void* res = mi_new(n);
    return res;
}

mi_decl_new_nothrow(n) void*
operator new(std::size_t n, const std::nothrow_t& tag) noexcept {
    (void) (tag);
    void* res = mi_new_nothrow(n);
    return res;
}
mi_decl_new_nothrow(n) void*
operator new[](std::size_t n, const std::nothrow_t& tag) noexcept {
    (void) (tag);
    void* res = mi_new_nothrow(n);
    return res;
}

#    if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void
operator delete(void* p, std::size_t n) noexcept {
    mi_free_size(p, n);
}
void
operator delete[](void* p, std::size_t n) noexcept {
    mi_free_size(p, n);
}
#    endif

#    if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void
operator delete(void* p, std::align_val_t al) noexcept {
    mi_free_aligned(p, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::align_val_t al) noexcept {
    mi_free_aligned(p, static_cast<size_t>(al));
}
void
operator delete(void* p, std::size_t n, std::align_val_t al) noexcept {
    mi_free_size_aligned(p, n, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept {
    mi_free_size_aligned(p, n, static_cast<size_t>(al));
}
void
operator delete(void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    mi_free_aligned(p, static_cast<size_t>(al));
}
void
operator delete[](void* p, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    mi_free_aligned(p, static_cast<size_t>(al));
}

void*
operator new(std::size_t n, std::align_val_t al) noexcept(false) {
    void* res = mi_new_aligned(n, static_cast<size_t>(al));
    return res;
}
void*
operator new[](std::size_t n, std::align_val_t al) noexcept(false) {
    void* res = mi_new_aligned(n, static_cast<size_t>(al));
    return res;
}
void*
operator new(std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    void* res = mi_new_aligned_nothrow(n, static_cast<size_t>(al));
    return res;
}
void*
operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
    (void) tag;
    void* res = mi_new_aligned_nothrow(n, static_cast<size_t>(al));
    return res;
}
#    endif
#  endif

#  ifdef _MSC_VER
#    pragma warning(pop)
#  endif

#endif
#endif // MIMALLOC_NEW_DELETE_H
