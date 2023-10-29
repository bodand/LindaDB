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

add_library(LindaDB STATIC
            public/ldb/data/chunked_list.hxx
            public/ldb/index/tree/payload/chime_payload.hxx
            public/ldb/index/tree/payload/scalar_payload.hxx
            public/ldb/index/tree/payload/vector_payload.hxx
            public/ldb/index/tree/index_query.hxx
            public/ldb/index/tree/payload.hxx
            public/ldb/index/tree/payload_dispatcher.hxx
            public/ldb/index/tree/tree.hxx
            public/ldb/index/tree/tree_node.hxx
            public/ldb/index/tree/tree_node_handler.hxx
            public/ldb/lv/linda_tuple.hxx
            public/ldb/lv/linda_value.hxx
            public/ldb/profiler.hxx
            public/ldb/query_tuple.hxx
            public/ldb/store.hxx
            src/data/chunked_list.cxx
            src/index/tree/payload/chime_payload.cxx
            src/index/tree/payload/scalar_payload.cxx
            src/index/tree/payload/vector_payload.cxx
            src/index/tree/index_query.cxx
            src/index/tree/payload.cxx
            src/index/tree/payload_dispatcher.cxx
            src/index/tree/tree.cxx
            src/index/tree/tree_node.cxx
            src/index/tree/tree_node_handler.cxx
            src/lv/linda_tuple.cxx
            src/store.cxx)

if (LINDA_DB_USE_MIMALLOC)
    add_library(LindaDB-mimalloc STATIC
                src/malloc_overrider.cxx)
    target_include_directories(LindaDB-mimalloc
                               PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                               PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    target_link_libraries(LindaDB-mimalloc
                          PUBLIC mimalloc-static
                          PRIVATE internal-warnings internal-lto)
    target_compile_features(LindaDB-mimalloc PUBLIC cxx_std_20)
    target_compile_definitions(LindaDB-mimalloc
                               PUBLIC LINDA_DB_USE_MIMALLOC)

    list(APPEND PROJECT_TARGETS LindaDB-mimalloc)
    target_link_libraries(LindaDB PUBLIC LindaDB-mimalloc)
endif ()

target_include_directories(LindaDB
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(LindaDB
                      PRIVATE internal-coverage internal-warnings internal-lto)
target_compile_features(LindaDB PUBLIC cxx_std_20)

if (TARGET Tracy::TracyClient)
    target_link_libraries(LindaDB PRIVATE Tracy::TracyClient)
    target_compile_definitions(LindaDB PRIVATE LINDA_DB_PROFILER)
endif ()

list(APPEND PROJECT_TARGETS LindaDB)
