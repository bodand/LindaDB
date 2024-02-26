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
# test/LindaDB/CMakeLists.txt --
#   CMake script for handling the LindaDB project's test requirements.

add_covered_test(NAME LindaDB.Test CATCH
                 SOURCES
                 bcast/broadcast.test.cxx
                 data/chunked_list.test.cxx
                 lv/linda_tuple.test.cxx
                 lv/linda_value.test.cxx
                 lv/tuple_builder.test.cxx
                 query/concrete_tuple_query.test.cxx
                 tree/avl/scalar_avl.test.cxx
                 tree/avl/vector_avl.test.cxx
                 tree/avl/chime_avl.test.cxx
                 tree_payloads/chime_payload.test.cxx
                 tree_payloads/scalar_payload.test.cxx
                 tree_payloads/vector_payload.test.cxx
                 store.test.cxx
                 LIBRARIES LindaDB)

add_covered_test(NAME LindaDB.AssertTest CATCH
                 SOURCES assert_test.cxx
                 LIBRARIES LindaDB-NoAbort)
