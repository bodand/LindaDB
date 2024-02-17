# LindaDB project
#
# Copyright (c) 2023 András Bodor <bodand@pm.me>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# - Neither the name of the copyright holder nor the names of its contributors
#   may be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Originally created: 2023-09-27.
#
# src/core/CMakeLists.txt --
#   The CMakeLists file for managing the LindaDB project's main source
#   components.

set(LDB_COMMON_SOURCES
    public/ldb/bcast/null_broadcast.hxx
    public/ldb/data/chunked_list.hxx
    public/ldb/index/tree/payload/chime_payload.hxx
    public/ldb/index/tree/payload/scalar_payload.hxx
    public/ldb/index/tree/payload/vector_payload.hxx
    public/ldb/index/tree/impl/avl2/avl2_tree.hxx
    public/ldb/index/tree/index_query.hxx
    public/ldb/index/tree/payload.hxx
    public/ldb/index/tree/payload_dispatcher.hxx
    public/ldb/lv/linda_tuple.hxx
    public/ldb/lv/linda_value.hxx
    public/ldb/query/concrete_tuple_query.hxx
    public/ldb/query/make_matcher.hxx
    public/ldb/query/manual_fields_query.hxx
    public/ldb/query/match_type.hxx
    public/ldb/query/match_value.hxx
    public/ldb/query/meta_finder.hxx
    public/ldb/query/tuple_query.hxx
    public/ldb/store.hxx
    src/data/chunked_list.cxx
    src/index/tree/payload/chime_payload.cxx
    src/index/tree/payload/scalar_payload.cxx
    src/index/tree/payload/vector_payload.cxx
    src/index/tree/index_query.cxx
    src/index/tree/payload.cxx
    src/index/tree/payload_dispatcher.cxx
    src/lv/linda_tuple.cxx
    src/query/concrete_tuple_query.cxx
    src/query/manual_fields_query.cxx
    src/query/tuple_query.cxx
    src/common.cxx
    src/store.cxx)

add_library(LindaDB STATIC ${LDB_COMMON_SOURCES})
add_library(LindaDB-NoAbort STATIC ${LDB_COMMON_SOURCES})

target_compile_definitions(LindaDB-NoAbort PUBLIC LDB_ASSERT_NOABORT)

add_library(LindaDB-malloc STATIC
            src/malloc_overrider.cxx)
target_include_directories(LindaDB-malloc
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(LindaDB-malloc
                      PUBLIC $<$<BOOL:${LINDA_DB_USE_MIMALLOC}>:mimalloc-static>
                      PRIVATE internal-warnings internal-lto internal-language-level)
target_compile_definitions(LindaDB-malloc
                           PUBLIC $<$<BOOL:${LINDA_DB_USE_MIMALLOC}>:LINDA_DB_USE_MIMALLOC>)

list(APPEND PROJECT_TARGETS LindaDB-malloc)
target_link_libraries(LindaDB PUBLIC LindaDB-malloc)
target_link_libraries(LindaDB-NoAbort PUBLIC LindaDB-malloc)

target_include_directories(LindaDB
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(LindaDB
                      PRIVATE internal-coverage internal-warnings internal-lto internal-language-level)
target_include_directories(LindaDB-NoAbort
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(LindaDB-NoAbort
                      PRIVATE internal-coverage internal-warnings internal-lto internal-language-level)

if (TARGET Tracy::TracyClient)
    target_link_libraries(LindaDB PUBLIC Tracy::TracyClient)
    target_compile_definitions(LindaDB PUBLIC LINDA_DB_PROFILER)

    target_link_libraries(LindaDB-malloc PUBLIC Tracy::TracyClient)
    target_compile_definitions(LindaDB-malloc PUBLIC LINDA_DB_PROFILER)
endif ()

list(APPEND PROJECT_TARGETS LindaDB)
