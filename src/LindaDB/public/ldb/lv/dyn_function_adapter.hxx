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
 * Originally created: 2024-02-28.
 *
 * src/LindaRT/public/lrt/dyn_function_adapter --
 *   
 */
#ifndef LINDADB_DYN_FUNCTION_ADAPTER_HXX
#define LINDADB_DYN_FUNCTION_ADAPTER_HXX

#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

#include <ldb/common.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>
#include <ldb/lv/global_function_map.hxx>

namespace ldb::lv {
    template<class T>
    struct expecting_visitor {
        using call_type = std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;

        call_type
        operator()(call_type val) {
            return val;
        }

        template<class Other>
        [[noreturn]] call_type
        operator()(Other&& val) {
            std::ignore = std::forward<Other>(val);
            assert_that(false, "bad type received at runtime for dynamic call");
            std::abort();
        }
    };

    template<>
    struct expecting_visitor<const char*> {
        const char*
        operator()(const std::string& str) {
            return str.data();
        }

        template<class T>
        [[noreturn]] const char*
        operator()(T&& bad_value) {
            std::ignore = std::forward<T>(bad_value);
            assert_that(false, "bad type received at runtime for dynamic call");
            std::abort();
        }
    };

    struct dynamic_executor {
        virtual ldb::lv::linda_value
        execute(void* fn, const ldb::lv::linda_tuple& dyn_args) = 0;

        virtual ~dynamic_executor() = default;
    };

    template<class... Args>
    struct typed_tuple_builder;

    template<class R, class T, class... Args>
    struct typed_tuple_builder<R, T, Args...> : typed_tuple_builder<R, Args...> {
        template<std::forward_iterator It,
                 std::sentinel_for<It> SIt>
        std::tuple<T, Args...>
        build(It begin, SIt end) {
            assert_that(begin != end,
                        "linda_tuple ended prematurely");
            auto head_value = std::make_tuple(std::visit(expecting_visitor<T>{}, *begin));
            auto tail_value = typed_tuple_builder<R, Args...>::build(++begin, end);
            return std::tuple_cat(head_value, tail_value);
        }

    private:
        ldb::lv::linda_value
        execute(void* fn,
                const ldb::lv::linda_tuple& dyn_args) override {
            auto* typed_fn = std::bit_cast<R (*)(T, Args...)>(fn);
            auto typed_args = build(dyn_args.begin(), dyn_args.end());
            return std::apply(typed_fn, typed_args);
        }
    };

    template<class R>
    struct typed_tuple_builder<R> : dynamic_executor {
        template<std::forward_iterator It,
                 std::sentinel_for<It> SIt>
        std::tuple<>
        build(It begin, SIt end) {
            assert_that(begin == end,
                        "non-empty linda_tuple reached end of calculation");
            return std::make_tuple();
        }

        ldb::lv::linda_value
        execute(void* fn,
                const ldb::lv::linda_tuple& dyn_args) override {
            auto* typed_fn = std::bit_cast<R (*)()>(fn);
            auto typed_args = build(dyn_args.begin(), dyn_args.end());
            return std::apply(typed_fn, typed_args);
        }
    };

    struct dyn_function_adapter {
        template<class R, class... Args>
        explicit dyn_function_adapter(R (&fun)(Args...))
             : _exeuctor(std::make_unique<typed_tuple_builder<R, Args...>>()),
               _fn(std::bit_cast<stored_type>(&fun)) { }

        template<class T>
        explicit dyn_function_adapter(T&&)
             : _exeuctor(),
               _fn() { assert_that(false, "non-function object passed to dyn_function_adapter"); }

        ldb::lv::linda_value
        operator()(const ldb::lv::linda_tuple& args) {
            return _exeuctor->execute(_fn, args);
        }

    private:
        using stored_type = void*;

        std::unique_ptr<dynamic_executor> _exeuctor;
        stored_type _fn;
    };
}

#endif
