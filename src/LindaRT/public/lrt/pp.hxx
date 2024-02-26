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
 * Originally created: 2024-02-22.
 *
 * src/LindaRT/pp --
 *   Macros and other stuff for the eval related implementation details.
 */
#ifndef LINDADB_PP_HXX
#define LINDADB_PP_HXX

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/list/transform.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>

namespace lrt::meta {
    template<class T>
    struct ptr_if_function {
        template<class U = T>
        static decltype(auto)
        forward(U&& arg) { return std::forward<U>(arg); }
    };

    template<class R, class... Args>
    struct ptr_if_function<R (&)(Args...)> {
        template<class U = R (&)(Args...)>
        static decltype(auto)
        forward(U arg) { return &arg; }
    };
}

#define eval(...) eval(tuple_builder DISSECT_EVAL_CALL(__VA_ARGS__).build())

#define WRAP_PARENS(...) (__VA_ARGS__)
#define MAYBE_WITH_LEADING_COMMA(...) __VA_OPT__(, ) __VA_ARGS__
#define ZIP_WITH_STRINGIZED(s, ignored, ...) BOOST_PP_STRINGIZE(BOOST_PP_REMOVE_PARENS(__VA_ARGS__)) MAYBE_WITH_LEADING_COMMA(BOOST_PP_REMOVE_PARENS(__VA_ARGS__))
#define PREPROCESS_ARGS(s, ignored, elem) BOOST_PP_SEQ_TRANSFORM(ZIP_WITH_STRINGIZED, , BOOST_PP_IIF(BOOST_PP_IS_BEGIN_PARENS(elem), BOOST_PP_VARIADIC_SEQ_TO_SEQ, WRAP_PARENS)(elem))
#define STRIP(r, ignored, x) x
#define DISSECT_EVAL_CALL(...) BOOST_PP_LIST_FOR_EACH(STRIP, , BOOST_PP_LIST_TRANSFORM(PREPROCESS_ARGS, , BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)))

#endif
