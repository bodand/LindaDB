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
 * Originally created: 2024-02-19.
 *
 * test/test --
 *   
 */

#include <string>
#include <string_view>
#include <tuple>

#include <boost/preprocessor.hpp>

template<class... Args>
struct fn_call {
    template<class... CArgs>
    explicit fn_call(std::string_view name, CArgs&&... args) : _name(name), _params(std::forward<CArgs>(args)...) { }

    std::string_view _name;
    std::tuple<Args...> _params;
};

template<class... Args>
fn_call(std::string_view, Args...) -> fn_call<Args...>;

#define FN_CALL(sym) (fn_call(                                                                              \
       BOOST_PP_STRINGIZE(BOOST_PP_REMOVE_PARENS(BOOST_PP_SEQ_ELEM(0, BOOST_PP_VARIADIC_SEQ_TO_SEQ(sym)))), \
                                                 BOOST_PP_REMOVE_PARENS(BOOST_PP_SEQ_ELEM(1, BOOST_PP_VARIADIC_SEQ_TO_SEQ(sym)))))
#define ADD_PARENS(...) (__VA_ARGS__)
#define SYM_OR_FN_CALL(r, asd, sym) BOOST_PP_IIF(BOOST_PP_IS_BEGIN_PARENS(sym), FN_CALL, ADD_PARENS)(sym)

#define eval(...) eval_i(__VA_ARGS__)
#define eval_i(...)                                                                                                                                      \
    do {                                                                                                                                                 \
        const auto call_args = std::make_tuple BOOST_PP_SEQ_TO_TUPLE(BOOST_PP_SEQ_FOR_EACH(SYM_OR_FN_CALL, asd, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); \
    } while (0)

extern "C" __declspec(dllexport) int
foo(int x) {
    return x + 1;
}

#include <bit>
#include <iostream>

#include <windows.h>

int
main() {
    auto my_foo = std::bit_cast<int (*)(int)>(GetProcAddress(GetModuleHandleW(nullptr), "foo"));
    std::cout << "my_foo: " << my_foo(41) << "\n";

    int a = 12;
    std::cout << (foo) (2) << "\n\n";
    eval(1, (foo) (a), "ayy");
    std::cout << "-------\n";
    eval((foo) (1, 2));
}
