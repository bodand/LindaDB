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
# Originally created: 2023-11-06.
#
# src/LindaRT/CMakeLists.txt --
#   The CMakeLists file for managing the LindaRT project's main source
#   components.

find_package(MPI REQUIRED)

add_library(LindaRT STATIC
            include/lrt/bcast/manual_broadcast_handler.hxx
            include/lrt/bcast/mpi_request_vector_awaiter.hxx
        include/lrt/bcast/reduce_awaiter.hxx
            include/lrt/work_pool/work_factory.hxx
            include/lrt/communication_tags.hxx
            public/lrt/balancer/balancer.hxx
            public/lrt/balancer/balancer_if.hxx
            public/lrt/balancer/uniform_random_balancer.hxx
            public/lrt/serialize/tuple.hxx
            public/lrt/work_pool/work_if.hxx
            public/lrt/work_pool/work.hxx
            public/lrt/work_pool/work_pool.hxx
            public/lrt/work_pool/work_queue.hxx
            public/lrt/eval.hxx
            public/lrt/linda.hxx
            public/lrt/mpi_runtime.hxx
            public/lrt/mpi_thread_context.hxx
            public/lrt/runtime.hxx
            public/lrt/runtime_storage.hxx
            src/bcast/manual_broadcast_handler.cxx
            src/bcast/multi_thread_broadcast.cxx
            src/serialize/tuple.cxx
            src/work_pool/work/eval_work.cxx
            src/work_pool/work_factory.cxx
            src/mpi_runtime.cxx
            src/mpi_thread_context.cxx
            src/runtime.cxx
            src/runtime_storage.cxx)

add_library(LindaRT-malloc STATIC
            src/malloc_overrider.cxx)
target_include_directories(LindaRT-malloc
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(LindaRT-malloc
                      PUBLIC $<$<BOOL:${LINDA_DB_USE_MIMALLOC}>:mimalloc-static>
                      PRIVATE internal-warnings internal-lto internal-language-level)
target_compile_definitions(LindaRT-malloc
                           PUBLIC $<$<BOOL:${LINDA_DB_USE_MIMALLOC}>:LINDA_DB_USE_MIMALLOC>)

target_include_directories(LindaRT
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(LindaRT
                      PUBLIC LindaDB LindaRT-malloc internal-language-level
                      PRIVATE MPI::MPI_CXX internal-coverage internal-warnings internal-lto)
target_compile_definitions(LindaRT-malloc
                           PUBLIC $<$<BOOL:${LINDA_RT_BIG_ENDIAN}>:LINDA_RT_BIG_ENDIAN>)

add_library(LindaRT-main STATIC src/main.cxx)
target_include_directories(LindaRT-main
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(LindaRT-main
                      PRIVATE LindaDB internal-warnings internal-lto internal-language-level)
set(PROJECT_TARGETS "${PROJECT_TARGETS};LindaRT-malloc;LindaRT;LindaRT-main" PARENT_SCOPE)

