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
#ifndef LINDADB_EVAL_HXX
#define LINDADB_EVAL_HXX

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <ldb/common.hxx>
#include <ldb/lv/dyn_function_adapter.hxx>
#include <ldb/lv/global_function_map.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <ldb/lv/tuple_builder.hxx>

#define LDB_PARENS_INSTEAD(...) ()

#define LDB_EVAL_ARG_FN(r, data, arg) BOOST_PP_IIF(BOOST_PP_IS_BEGIN_PARENS(arg), arg, (arg))
#define LDB_EVAL_ARGS(...)               \
    __VA_OPT__(                          \
           BOOST_PP_VARIADIC_SEQ_TO_SEQ( \
                  BOOST_PP_LIST_FOR_EACH(LDB_EVAL_ARG_FN, (._.), BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__))))

#ifndef LDB_STR_I
#  define LDB_STR_I(...) #__VA_ARGS__
#endif
#define LDB_EVAL_REG_FN_I(arg) std::ignore = lrt::fn_mapper<LDB_STR_I(arg)>::value<decltype(arg),                                                              \
                                                                                   decltype([](const ldb::lv::linda_tuple& args) -> ldb::lv::linda_value {     \
                                                                                       LDBT_ZONE_A;                                                            \
                                                                                       if constexpr (std::is_function_v<std::remove_cvref_t<decltype(arg)>>) { \
                                                                                           ldb::lv::dyn_function_adapter adapter(arg);                         \
                                                                                           return adapter(args);                                               \
                                                                                       }                                                                       \
                                                                                       assert_that(false, "called a non-function type");                       \
                                                                                       return {};                                                              \
                                                                                   })>;

#define LDB_EVAL_REG_FN(r, data, arg) LDB_EVAL_REG_FN_I(BOOST_PP_REMOVE_PARENS(arg))
#define LDB_EVAL_REGISTER_FUNCTIONS(seq) \
    BOOST_PP_SEQ_FOR_EACH(LDB_EVAL_REG_FN, (T_T), seq)

#define LDB_EVAL_CALL_FN_I(arg) (LDB_STR_I(arg), arg)
#define LDB_EVAL_CALL_FN(r, data, arg) LDB_EVAL_CALL_FN_I(BOOST_PP_REMOVE_PARENS(arg))
#define LDB_EVAL_BUILD_CALL_I(seq)      \
    BOOST_PP_IF(BOOST_PP_SEQ_SIZE(seq), \
                BOOST_PP_SEQ_FOR_EACH,  \
                LDB_PARENS_INSTEAD)     \
    (LDB_EVAL_CALL_FN, , seq)
#define LDB_EVAL_BUILD_CALL_TUPLE(seq) \
    ldb::lv::tuple_builder             \
    LDB_EVAL_BUILD_CALL_I(seq)         \
           .build()

#define eval(...)                                                                        \
    do {                                                                                 \
        LDB_EVAL_REGISTER_FUNCTIONS(LDB_EVAL_ARGS(__VA_ARGS__))                          \
        const auto call_payload = LDB_EVAL_BUILD_CALL_TUPLE(LDB_EVAL_ARGS(__VA_ARGS__)); \
        lrt::this_runtime().eval(call_payload);                                          \
    } while (0)

namespace lrt {
    template<std::size_t N>
    struct constexpr_string_view {
        constexpr explicit(false) constexpr_string_view(const char (&str)[N]) {
            std::ranges::copy(str, std::begin(value));
        }

        [[nodiscard]] std::size_t
        size() const noexcept { return N; }
        [[nodiscard]] const char*
        data() const noexcept { return static_cast<const char*>(value); }

        char value[N]{};
    };

    template<constexpr_string_view Name,
             class DispatchType,
             class T>
        requires(std::is_invocable_r_v<ldb::lv::linda_value, T, const ldb::lv::linda_tuple&>)
    struct function_loader {
        function_loader() = default;
    };

    template<constexpr_string_view Name,
             class R,
             class... Args,
             class T>
        requires(std::is_invocable_r_v<ldb::lv::linda_value, T, const ldb::lv::linda_tuple&>)
    struct function_loader<Name, R(Args...), T> {
        function_loader() {
            LDBT_ZONE_A;
            if (!ldb::lv::gLdb_Dynamic_Function_Map) ldb::lv::gLdb_Dynamic_Function_Map = std::make_unique<ldb::lv::global_function_map_type>();
            auto fn_name = std::string{Name.data(), Name.size() - 1};
            (*ldb::lv::gLdb_Dynamic_Function_Map).emplace(fn_name, T{});
        }
    };

    template<constexpr_string_view fun>
    struct fn_mapper {
        template<class DispatchType, class callee>
        inline static function_loader<fun, DispatchType, callee> value;
    };
}

#endif
