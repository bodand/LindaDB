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
# cmake/add_htmldoc.cmake --
#   Function for creating html formatted documentation from the AsciiDoc files
#   in the repository.
#   HTML docs are mainly the developer documenation.

function(add_htmldoc html_file src_file)
    set(input_file "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
    set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${html_file}")
    string(MAKE_C_IDENTIFIER "${output_file}" output_target)

    add_custom_command(OUTPUT "${output_file}"
                       DEPENDS "${input_file}"
                       COMMENT "Building html documentation page ${html_file}"
                       VERBATIM COMMAND
                       "${ASCIIDOCTOR_EXE}"
                       -b html5
                       "${input_file}"
                       -o "${output_file}"
                       )
    add_custom_target("${output_target}" ALL
                      DEPENDS "${output_file}")
endfunction()
