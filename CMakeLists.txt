# </project/> project
#
# </license_text/>
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
project(</project/>
        VERSION 0.0.1
        DESCRIPTION "</summary/>"
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
find_program(PANDOC_EXE pandoc
             DOC "The pandoc executable used to convert documentation formats")

# coverage data
add_library(internal-coverage INTERFACE)
if (NOT DEFINED ENV{CG_DEPLOYMENT})
    target_compile_options(internal-coverage INTERFACE
                           $<$<CXX_COMPILER_ID:Clang>:-fprofile-instr-generate>
                           $<$<CXX_COMPILER_ID:Clang>:-fcoverage-mapping>)
    if ("x${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "xMSVC")
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH vs_llvmx64_bindir)
        cmake_path(GET vs_llvmx64_bindir PARENT_PATH vs_llvmx64_dir)
        set(clang_profile_lib "${vs_llvmx64_dir}/lib/clang/15.0.1/lib/windows/clang_rt.profile-x86_64.lib")
        target_link_libraries(internal-coverage INTERFACE "${clang_profile_lib}")
    else ()
        target_link_options(internal-coverage INTERFACE
                            $<$<CXX_COMPILER_ID:Clang>:-fprofile-instr-generate>
                            $<$<CXX_COMPILER_ID:Clang>:-fcoverage-mapping>)
    endif ()
endif ()

# warnings
add_library(internal-warnings INTERFACE)
generate_warnings(internal-warnings)

# LTO
add_library(internal-lto INTERFACE)
generate_lto(internal-lto)

# Dependencies
if ("x${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "xMSVC")
    message(STATUS "Using clang-cl: ${CMAKE_CXX_COMPILER}")
    message(STATUS "Injecting /EHsc into clang-cl command-line")
    add_compile_options(/EHsc)
endif ()

# targets
set(PROJECT_TARGETS)
#add_subdirectory(src/tgt1)
# docs
#add_subdirectory(docs)
# test
#add_subdirectory(test)

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

# Installation
#install(TARGETS ...
#        DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.adoc"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}")

# cpack
include(cmake/cpack_config.cmake)