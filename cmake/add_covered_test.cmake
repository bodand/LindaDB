# cg3 project
#
# Copyright (c) 2022, András Bodor <bodand@proton.me>
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
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# cmake/add_covered_test.cmake --
#   Adds a test that is to be expected to run using coverage tools when built with
#   a clang-like thing.

function(add_covered_test)
    cmake_parse_arguments(PARSE_ARGV 0 act
            "CATCH"  # FLAGS
            "NAME"  # SINGLE ARGS
            "SOURCES;LIBRARIES") # MULTI ARGS

    if (NOT DEFINED act_NAME
            OR NOT DEFINED act_SOURCES)
        message(SEND_ERROR "Invalid call to add_covered_test(NAME <name> [CATCH]
        SOURCES <src>...
        [LIBRARIES <lib>...])")
        return()
    endif ()

    set(test_name "${act_NAME}")
    set(test_data "${act_NAME}-data")

    add_executable("${test_name}" ${act_SOURCES})
    target_compile_features("${test_name}" PRIVATE cxx_std_20)
    target_link_libraries("${test_name}" PRIVATE ${act_LIBRARIES})

    if (act_CATCH)
        catch_discover_tests("${test_name}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=%m-%p.profraw")
    else ()
        add_test(NAME "${test_name}"
                COMMAND "${test_name}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        set_tests_properties("${test_name}" PROPERTIES
                ENVIRONMENT "LLVM_PROFILE_FILE=%m-%p.profraw")
    endif ()
endfunction()
