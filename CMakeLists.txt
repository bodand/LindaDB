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

cmake_minimum_required(VERSION 3.22)
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
        VERSION 0.0.1
        DESCRIPTION "HPC-centric, distributed IMDB for the linda language's implementation."
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
include(CTest)

# our modules
include(cmake/add_htmldoc.cmake)
include(cmake/add_manpage.cmake)
include(cmake/add_covered_test.cmake)
include(cmake/generate_warnings.cmake)
include(cmake/generate_lto.cmake)

find_program(ASCIIDOCTOR_EXE asciidoctor
        DOC "The asciidoctor executable used to build documentation")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32)
    set(LDB_DEFAULT_PROFILER NO)
else ()
    set(LDB_DEFAULT_PROFILER YES)
endif ()

option(LINDA_DB_USE_PROFILER
        "While building the LindaDB implementation provide the required orchestration for the Tracy profiler [${LDB_DEFAULT_PROFILER}]"
        ${LDB_DEFAULT_PROFILER})
option(LINDA_DB_USE_MIMALLOC
        "Build LindaDB using the mimalloc allocator instead of the system's [YES]"
        NO)

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
        set(clang_profile_lib "${vs_llvmx64_dir}/lib/clang/16/lib/windows/clang_rt.profile-x86_64.lib")
        target_link_libraries(internal-coverage INTERFACE "${clang_profile_lib}")
    else ()
        target_link_options(internal-coverage INTERFACE
                $<$<CXX_COMPILER_ID:Clang>:-fprofile-instr-generate>
                $<$<CXX_COMPILER_ID:Clang>:-fcoverage-mapping>)
    endif ()
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug"
        AND CMAKE_CXX_COMPILER_ID MATCHES [[GNU|Clang]])
    add_compile_options(
            $<$<PLATFORM_ID:Linux>:-fsanitize=address>
            $<$<PLATFORM_ID:Linux>:-fno-omit-frame-pointer>
    )
    add_link_options(
            $<$<PLATFORM_ID:Linux>:-fsanitize=address,undefined>
    )
endif ()

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
find_package(mimalloc CONFIG REQUIRED)
find_package(Catch2 CONFIG)
if (LINDA_DB_USE_PROFILER)
    find_package(Tracy CONFIG REQUIRED)
endif ()

# targets
set(PROJECT_TARGETS)
add_subdirectory(src/LindaDB)
# docs
#add_subdirectory(docs)
# test
add_subdirectory(test)

# clang-tidy
if (CMAKE_GENERATOR MATCHES [[Make|Ninja]]
        AND (NOT DEFINED ENV{CI} OR DEFINED ENV{CI_DO_CLANG_TIDY}))
    # CMAKE_EXPORT_COMPILE_COMMANDS is only implemented on Make and Ninja...
    cmake_path(RELATIVE_PATH CMAKE_CURRENT_BINARY_DIR OUTPUT_VARIABLE rel_bindir)
    foreach (tgt IN LISTS PROJECT_TARGETS)
        get_target_property(tgt_dir "${tgt}" SOURCE_DIR)
        get_target_property(tgt_srces "${tgt}" SOURCES)
        get_target_property(tgt_isrces "${tgt}" INTERFACE_SOURCES)
        foreach (src IN LISTS tgt_srces tgt_isrces)
            if (NOT src)
                continue()
            endif ()
            cmake_path(ABSOLUTE_PATH src BASE_DIRECTORY "${tgt_dir}")
            cmake_path(RELATIVE_PATH src BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

            add_test(NAME "clang-tidy whole project/${src}"
                    COMMAND "clang-tidy" -p "${rel_bindir}" ${src}
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    COMMAND_EXPAND_LISTS)
        endforeach ()
    endforeach ()
endif ()
