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
# cmake/cpack_config.cmake --
#   Configuration required by cpack. Includes the CPack module as well.

set(CPACK_PACKAGE_NAME "</project/>")
set(CPACK_PACKAGE_FILE_NAME "</project/>-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${PACKAGE_ARCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "</project/>-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${PACKAGE_ARCH}")
set(CPACK_THREADS 0)
set(CPACK_PACKAGE_VENDOR "</name/> <</email/>>")
set(CPACK_PACKAGE_CONTACT "</name/> <</email/>>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "</summary/>")
set(CPACK_PACKAGE_DESCRIPTION "</description/>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.adoc")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.adoc")
set(CPACK_SOURCE_IGNORE_FILES
    "_build.*"
    "\\\\\\\\.idea.*"
    "\\\\\\\\.git/modules/.*"
    "vcpkg/.*"
    )
set(CPACK_STRIP_FILES TRUE)
if (WIN32)
    if (NOT ASCIIDOCTOR_EXE OR NOT PANDOC_EXE)
        message(SEND_ERROR "Cannot build Windows package without asciidoctor and pandoc")
    endif ()
    set(CPACK_GENERATOR WIX)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.docbook"
                       VERBATIM COMMAND "${ASCIIDOCTOR_EXE}" -b docbook
                       "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.adoc"
                       -o "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.docbook"
                       DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.adoc")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.rtf"
                       VERBATIM COMMAND "${PANDOC_EXE}"
                       -f docbook
                       -s "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.docbook"
                       -o "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.rtf"
                       DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.docbook")
    add_custom_target(LicenseRTF ALL
                      DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.rtf")

    set(build_triplet "${BUILD_SYS_ARCH}-${BUILD_SYS_NAME}")
    file(GLOB DEPENDENCY_LIBS
         LIST_DIRECTORIES FALSE
         CONFIGURE_DEPENDS
         "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${build_triplet}/bin/*.dll")
    list(FILTER DEPENDENCY_LIBS EXCLUDE REGEX [[(libclang|LLVM-C|Remarks|LTO)\.dll]])
    install(FILES ${DEPENDENCY_LIBS}
            DESTINATION "${CMAKE_INSTALL_BINDIR}")

    set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.rtf")
    set(CPACK_WIX_UPGRADE_GUID "7F734678-F8F4-4CD9-A09F-09BBB09EACD3")
endif ()

# Debian package
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/copyright"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}")
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# RPM package
set(CPACK_RPM_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.rpm")
set(CPACK_RPM_PACKAGE_LICENSE "BSD")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_COMPRESSION_TYPE "xz")

include(CPack)
