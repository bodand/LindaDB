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
# Originally created: 2023-10-14.
#
# test/CMakeLists.txt --
#   The main orchestrator project file for handling the test-cases of LindaDB.

if (NOT TARGET Catch2::Catch2)
    message(STATUS "Catch2 not present -- skipping tests")
    return()
endif ()

find_path(FAKEIT_INCLUDE_DIRS "boost/fakeit.hpp")
add_library(fakeit INTERFACE)
target_include_directories(fakeit INTERFACE "${FAKEIT_INCLUDE_DIRS}")

include(Catch)

add_subdirectory(LindaDB)
add_subdirectory(LindaRT)

find_package(MPI REQUIRED)
add_executable(lindart-test lindart.cxx)
target_link_libraries(lindart-test PUBLIC MPI::MPI_CXX LindaRT)

add_test(NAME "LindaRT-MPI"
         COMMAND "${MPIEXEC_EXECUTABLE}" "${MPIEXEC_NUMPROC_FLAG}" "${MPIEXEC_MAX_NUMPROCS}" $<TARGET_FILE:lindart-test>)