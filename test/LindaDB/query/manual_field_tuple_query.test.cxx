/* LindaDB project
 *
 * Copyright (c) 2024 András Bodor <bodand@pm.me>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the copyright holder nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Originally created: 2024-04-04.
 *
 * test/LindaDB/query/manual_field_tuple_query --
 *   
 */


#include <concepts>

#include <catch2/catch_test_macros.hpp>
#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query.hxx>

using index_type = ldb::index::tree::avl2_tree<int, int>;

TEST_CASE("fully-specified query returns full-tuple as representing tuple") {
    const auto query = ldb::make_piecewise_query(ldb::over_index<index_type>,
                                                 1,
                                                 2,
                                                 3);
    const auto expected = ldb::lv::linda_tuple(1, 2, 3);

    CHECK(query.as_representing_tuple() == expected);
}

TEST_CASE("ref containing query returns ref_type in tuple as representing tuple") {
    int x = 10;
    const auto query = ldb::make_piecewise_query(ldb::over_index<index_type>,
                                                 1,
                                                 ldb::ref(&x),
                                                 3);
    const auto expected = ldb::lv::linda_tuple(1, ldb::lv::ref_type(2), 3);

    CHECK(query.as_representing_tuple() == expected);
}
