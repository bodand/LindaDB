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
 * Originally created: 2024-03-02.
 *
 * test/LindaDB/lv/fn_call_tag --
 *   
 */


#include <cstring>
#include <memory>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <ldb/lv/fn_call_tag.hxx>

using namespace ldb;

TEST_CASE("fn_call_tags compare equal") {
    CHECK(lv::fn_call_tag{} == lv::fn_call_tag{});
}

TEST_CASE("fn_call_tags don't compare less") {
    CHECK_FALSE(lv::fn_call_tag{} < lv::fn_call_tag{});
}

TEST_CASE("fn_call_tags don't compare greater") {
    CHECK_FALSE(lv::fn_call_tag{} > lv::fn_call_tag{});
}

TEST_CASE("fn_call_tags don't compare equal to anything") {
    CHECK_FALSE(lv::fn_call_tag{} == 0);
}

TEST_CASE("fn_call_tags compare less to anything") {
    CHECK(lv::fn_call_tag{} < 0);
}

TEST_CASE("fn_call_tags don't compare greater to anything") {
    CHECK_FALSE(lv::fn_call_tag{} > 0);
}
