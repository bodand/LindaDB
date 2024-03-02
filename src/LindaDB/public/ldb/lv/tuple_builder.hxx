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
 * src/LindaDB/public/ldb/lv/tuple_builder --
 *   A mutable tuple-builder for the immutable linda_tuple objects.
 */
#ifndef LINDADB_TUPLE_BUILDER_HXX
#define LINDADB_TUPLE_BUILDER_HXX

#include <concepts>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <ldb/lv/fn_call_holder.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>

namespace ldb::lv {
    struct bad_tuple_build : std::runtime_error {
        explicit bad_tuple_build(std::string_view fn_name)
             : std::runtime_error("invalid build when waiting for function arguments"),
               _fn_name(fn_name) { }

        std::string _fn_name;
    };

    struct tuple_builder {
        tuple_builder() = default;

        template<class T>
        tuple_builder(std::string_view name, T&& arg) { // initial call
            adder_impl<T>::add(*this, name, std::forward<T>(arg));
        }

        template<class T>
        tuple_builder&
        operator()(std::string_view name, T&& arg) {
            adder_impl<T>::add(*this, name, std::forward<T>(arg));
            return *this;
        }

        template<class... Args>
        tuple_builder&
        operator()(std::string_view name, Args&&... args) {
            std::ignore = name;
            add_arguments(make_linda_value(std::forward<Args>(args))...);
            return *this;
        }

        auto
        build() {
            if (_last_fn) throw bad_tuple_build(_last_fn->function_name);
            return linda_tuple(_values);
        }

    private:
        struct last_fun_type {
            std::string function_name;
            linda_value typeinfo;
        };

        template<class T>
        struct adder_impl {
            template<class U = T>
            static void
            add(tuple_builder& builder, std::string_view ignore, U&& arg) {
                std::ignore = ignore;
                builder.add_value(std::forward<U>(arg));
            }
        };

        template<class R, class... Args>
        struct adder_impl<R(&)(Args...)> {
            template<class Fn>
            static void
            add(tuple_builder& builder, std::string_view fn_name, Fn&& fn) {
                std::ignore = std::forward<Fn>(fn);
                builder.add_function<R>(fn_name);
            }
        };

        template<class... Args>
        void
        add_arguments(Args&&... args)
            requires((std::same_as<Args, linda_value> && ...))
        {
            if (!_last_fn) throw bad_tuple_build("<missing>");
            add_fn_call(linda_tuple(std::forward<Args>(args)...));
            _last_fn.reset();
        }

        template<class T>
        void
        add_value(T&& arg) {
            if (_last_fn) {
                add_fn_call(linda_tuple(std::forward<T>(arg)));
                _last_fn.reset();
                return;
            }

            _values.emplace_back(std::forward<T>(arg));
        }

        void
        add_fn_call(linda_tuple&& tuple);

        template<class R>
        void
        add_function(std::string_view fn) {
            if (_last_fn) throw bad_tuple_build(fn);
            _last_fn = last_fun_type{.function_name = {fn.data(), fn.size()},
                                     .typeinfo = R{}};
        }

        std::optional<last_fun_type> _last_fn = std::nullopt;
        std::vector<linda_value> _values{};
    };
}

#endif
