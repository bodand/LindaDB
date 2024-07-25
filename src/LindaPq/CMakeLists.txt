# LindaDB project
#
# Copyright (c) 2024 András Bodor <bodand@proton.me>
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
# Originally created: 2024-07-07.
#
# src/LindaPq/CMakeLists.txt --
#   Postgres based db backend: contains two subprojects:
#    - pglinda: a PostgreSQL plugin, which provides the custom datatypes required
#               for modelling the Linda data-model in Postgres, uses Postgres
#               server development files
#    - lindapq: a static library, that is linked into LindaRT providing the
#               postgres based backend to LindaRT, uses libpq

if (NOT LDB_HAVE_POSTGRES)
    message(DEBUG "Postgres libraries not found, skipping Postgres backend")
    return()
endif ()

if (NOT LDB_HAVE_FAM_IN_CXX)
    message(DEBUG "FAM in C++ is required, skipping Postgres backend")
    return()
endif ()

enable_language(C)

add_library(pglinda SHARED
        pglinda/include/fam_datum.hxx
        pglinda/include/linda_funs.h
        pglinda/src/linda_binary.cxx
        pglinda/src/linda_compare.cxx
        pglinda/src/linda_hash.cxx
        pglinda/src/linda_str.cxx
        pglinda/src/linda_utils.cxx
        pglinda/src/pq_interface.c
)
target_link_libraries(pglinda
        PUBLIC PostgreSQL::PostgreSQL LindaDB
        PRIVATE internal-coverage internal-language-level internal-warnings internal-lto xxHash::xxhash)
