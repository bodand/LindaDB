# cg3 project
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
# Originally created: 2023-04-29.
#
# cmake/generate_lto.cmake --
#  Configuration for LTO

include(CheckIPOSupported)

option(</project/>_FORBID_LTO "Instruct the build-system to never enable LTO [No]" No)

function(generate_lto _tgt)
    message(CHECK_START "Checking for LTO support")
    if (</project/>_FORBID_LTO)
        message(CHECK_FAIL "forbidden")
        return()
    endif ()

    if (NOT CMAKE_BUILD_TYPE MATCHES [[Rel]])
        message(CHECK_FAIL "skip: not in release mode")
        return()
    endif ()

    check_ipo_supported(RESULT HAVE_LTO)
    if (NOT HAVE_LTO)
        message(CHECK_FAIL "failed")
    endif ()
    message(CHECK_PASS "found")

    set_target_properties("${_tgt}" PROPERTIES
                          INTERFACE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    if (MSVC)
        set_target_properties("${_tgt}" PROPERTIES
                              INTERFACE_COMPILE_OPTIONS "/GL"
                              INTERFACE_LINK_FLAGS "/LTCG:incremental")
    endif ()
endfunction()
