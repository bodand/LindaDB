# LindaDB project
#
# Copyright (c) 2023 bodand <bodand@proton.me>. 
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   1. Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#
#   2. Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#
#   3. Neither the name of the copyright holder nor the names of its
#      contributors may be used to endorse or promote products derived from this
#      software without specific prior written permission.
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
# CMakeLists.txt --
#   The main CMake script for building and managing the project.

cmake_minimum_required(VERSION 3.26)
enable_language(C)
enable_language(CXX)

# CMake policies ###############################################################
# Set CMake policies to the expected behavior.

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
endif ()
cmake_policy(SET CMP0127 NEW)
cmake_policy(SET CMP0125 NEW)
cmake_policy(SET CMP0118 NEW)
cmake_policy(SET CMP0115 NEW)
cmake_policy(SET CMP0111 NEW)
cmake_policy(SET CMP0110 NEW)
cmake_policy(SET CMP0096 NEW)
cmake_policy(SET CMP0092 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0065 NEW)
cmake_policy(SET CMP0048 NEW)

# project #########################################################################
project(LindaDB
        VERSION 0.1.1
        DESCRIPTION "Distributed IMDB for the linda language's implementation."
        LANGUAGES CXX)

# generate versioninfo.txt file for build
# do not remove, needed for CI/CD
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/versioninfo.txt" "${PROJECT_VERSION}")

# Include modules
include(GNUInstallDirs)
include(CheckCXXSymbolExists)
include(CMakeDependentOption)
include(InstallRequiredSystemLibraries)
include(FetchContent)
include(TestBigEndian)
include(CTest)

# our modules
include(cmake/add_htmldoc.cmake)
include(cmake/add_manpage.cmake)
include(cmake/add_covered_test.cmake)
include(cmake/generate_warnings.cmake)
include(cmake/generate_lto.cmake)

find_program(ASCIIDOCTOR_EXE asciidoctor
        DOC "The asciidoctor executable used to build documentation")

test_big_endian(LRT_IS_BIG_ENDIAN)

option(LINDA_DB_USE_MIMALLOC
        "Build LindaDB using the mimalloc allocator instead of the system's [YES]"
        YES)
option(LINDA_RT_BIG_ENDIAN
        "Serialize Linda values in a big-endian byte-order. [IF SYSTEM IS BE]"
        ${LRT_IS_BIG_ENDIAN})
option(LINDA_DB_PROFILING
        "Build with profiling enabled (Tracy profiler) [YES]"
        YES)

option(LINDA_DB_POSTGRES
        "Build the Postgres based DB-backend for LindaDB [YES] (Requires full Postgres server libraries, ignored if not found)"
        YES)

# coverage data
add_library(internal-coverage INTERFACE)
if (NOT DEFINED ENV{CG_DEPLOYMENT})
    target_compile_options(internal-coverage INTERFACE
            $<$<CXX_COMPILER_ID:Clang>:-fprofile-instr-generate>
            $<$<CXX_COMPILER_ID:Clang>:-fcoverage-mapping>)
    if ("x${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "xMSVC"
            AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH vs_llvmx64_bindir)
        cmake_path(GET vs_llvmx64_bindir PARENT_PATH vs_llvmx64_dir)
        if (CMAKE_CXX_COMPILER_VERSION MATCHES [[^([1-9][0-9]*)\.]])
            set(clang_profile_lib "${vs_llvmx64_dir}/lib/clang/${CMAKE_MATCH_1}/lib/windows/clang_rt.profile-x86_64.lib")
            target_link_libraries(internal-coverage INTERFACE "${clang_profile_lib}")
        else ()
            message(WARNING
                    "Cannot deduce Clang Runtime location: cannot calculate the major version of the compiler is from: ${CMAKE_CXX_COMPILER_VERSION}")
        endif ()
    else ()
        target_link_options(internal-coverage INTERFACE
                $<$<CXX_COMPILER_ID:Clang>:-fprofile-instr-generate>
                $<$<CXX_COMPILER_ID:Clang>:-fcoverage-mapping>)
    endif ()
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug"
        AND CMAKE_CXX_COMPILER_ID MATCHES [[GNU|Clang]])
    #    add_compile_options(
    #            $<$<PLATFORM_ID:Linux>:-fsanitize=address>
    #            $<$<PLATFORM_ID:Linux>:-fno-omit-frame-pointer>
    #    )
    #    add_link_options(
    #            $<$<PLATFORM_ID:Linux>:-fsanitize=address,undefined>
    #    )
endif ()

try_compile(LDB_HAVE_CXX23
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/try_compile/have_cxx23.cxx"
        LOG_DESCRIPTION "Checking for C++23 support..."
        CXX_STANDARD 23
        CXX_EXTENSIONS NO)
try_compile(LDB_HAVE_STACKTRACE
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/try_compile/have_stacktrace.cxx"
        LOG_DESCRIPTION "Checking for std::stacktrace support..."
        CXX_STANDARD 23
        CXX_EXTENSIONS NO)
try_compile(LDB_HAVE_FAM_IN_CXX
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/try_compile/have_fam.cxx"
        LOG_DESCRIPTION "Checking for FAM support in C++..."
        CXX_STANDARD 23
        CXX_EXTENSIONS NO)
add_library(internal-language-level INTERFACE)
if (LDB_HAVE_CXX23)
    set(LDB_CXX_FILE_SUFFIX cxx23)
    target_compile_features(internal-language-level INTERFACE cxx_std_23)
else ()
    set(LDB_CXX_FILE_SUFFIX cxx20)
    target_compile_features(internal-language-level INTERFACE cxx_std_20)
endif ()
target_compile_definitions(internal-language-level INTERFACE
        LDB_HAVE_STACKTRACE=0$<BOOL:${LDB_HAVE_STACKTRACE}>)

# warnings
add_library(internal-warnings INTERFACE)
generate_warnings(internal-warnings)

# LTO
add_library(internal-lto INTERFACE)
generate_lto(internal-lto)

# Dependencies
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Using MSVC: ${CMAKE_CXX_COMPILER}")
    message(STATUS "Injecting /permissive- into cl command-line")
    add_compile_options(/permissive-)
    message(STATUS "Injecting /Zc:__cplusplus into cl command-line")
    add_compile_options(/Zc:__cplusplus)
endif ()
if ("x${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "xMSVC"
        AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Using clang-cl: ${CMAKE_CXX_COMPILER}")
    message(STATUS "Injecting /EHsc into clang-cl command-line")
    add_compile_options(/EHsc)
endif ()
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

find_package(mimalloc CONFIG REQUIRED)
find_package(Catch2 CONFIG)
find_package(Boost REQUIRED)
find_package(Tracy CONFIG)

if (LINDA_DB_POSTGRES)
    find_package(xxHash CONFIG REQUIRED)
    find_package(PostgreSQL)
    if (TARGET PostgreSQL::PostgreSQL)
        set(LDB_HAVE_POSTGRES YES CACHE BOOL "Whether Postgres was detected")
    else ()
        set(LDB_HAVE_POSTGRES NO CACHE BOOL "Whether Postgres was detected")
    endif ()
endif ()

add_library(ldb-tracy INTERFACE)
if (TARGET Tracy::TracyClient)
    target_link_libraries(ldb-tracy INTERFACE Tracy::TracyClient)
endif ()

# Linda macro
macro(add_linda_executable exe)
    add_executable("${exe}" ${ARGN})
    target_link_libraries("${exe}" PRIVATE LindaRT LindaRT-main)
endmacro()

# targets
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

set(PROJECT_TARGETS)
add_subdirectory(src/LindaDB)
add_subdirectory(src/LindaPq)
add_subdirectory(src/LindaRT)
# demo apps
add_subdirectory(demo)
# docs
#add_subdirectory(docs)
# test
add_subdirectory(test)
