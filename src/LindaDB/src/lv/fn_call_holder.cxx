
#include "ldb/lv/fn_call_holder.hxx"

#include "ldb/lv/linda_tuple.hxx"

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
 * Originally created: 2024-02-26.
 *
 * src/LindaDB/src/lv/fn_call_holder --
 *   
 */

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include <ldb/common.hxx>
#include <ldb/lv/fn_call_holder.hxx>
#include <ldb/lv/global_function_map.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/tuple_builder.hxx>

ldb::lv::fn_call_holder::fn_call_holder(std::string fn_name, std::unique_ptr<linda_tuple>&& tuple)
     : _fn_name(std::move(fn_name)), _args(std::move(tuple)) {
    assert_that(_args, "fn_call_holder: null may not be passed as the args tuple");
}

ldb::lv::fn_call_holder::~fn_call_holder() = default;

ldb::lv::fn_call_holder::fn_call_holder(const fn_call_holder& cp)
     : _fn_name(cp._fn_name),
       _args(cp._args->clone()) { }

ldb::lv::fn_call_holder&
ldb::lv::fn_call_holder::operator=(const ldb::lv::fn_call_holder& cp) {
    if (&cp == this) return *this;
    _fn_name = cp._fn_name;
    _args = cp._args->clone();
    return *this;
}

ldb::lv::linda_tuple
ldb::lv::fn_call_holder::execute(int after_prefix, const linda_tuple& elements) {
    assert_that(gLdb_Dynamic_Function_Map != nullptr,
                "dynamic execution was not initialized: maybe no functions are dynamically invocable?");
    auto it = gLdb_Dynamic_Function_Map->find(_fn_name);
    assert_that(it != gLdb_Dynamic_Function_Map->end(),
                "dynamic function was not registered in dynamic execution subsystem");

    auto call_result = it->second(*_args);

    auto tuple_b = tuple_builder();
    auto tuple_appender = [&tuple_b](auto&& arg) {
        tuple_b("<ignored>", std::forward<decltype(arg)>(arg));
    };

    std::ranges::for_each_n(elements.begin(), after_prefix, tuple_appender);
    tuple_b("<ignored>", call_result);
    std::ranges::for_each(std::next(elements.begin(), after_prefix), elements.end(), tuple_appender);

    return tuple_b.build();
}

ldb::lv::fn_call_holder::fn_call_holder(ldb::lv::fn_call_holder&& mv) noexcept = default;

ldb::lv::fn_call_holder&
ldb::lv::fn_call_holder::operator=(ldb::lv::fn_call_holder&& mv) noexcept = default;
