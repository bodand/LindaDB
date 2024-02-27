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
 * Originally created: 2024-02-27.
 *
 * src/LindaDB/src/lv/fn_call_builder --
 *   
 */

#include <memory>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <variant>

#include <ldb/loader.hxx>
#include <ldb/lv/fn_call_builder.hxx>
#include <ldb/lv/linda_value.hxx>

namespace {
    using namespace ldb;

    template<class... Args>
    struct typed_call_builder;

    template<class... Args>
    struct typed_builder_building_visitor {
        explicit typed_builder_building_visitor(typed_call_builder<Args...>& builder)
             : builder(builder) { }

        template<class T>
        std::unique_ptr<lv::fn_call_builder>
        operator()(T&& obj) const {
            return builder.add_typed_arg(std::forward<T>(obj));
        }

        typed_call_builder<Args...>& builder;
    };

    template<class NewArg, class... LastArgs>
    auto
    append(std::tuple<LastArgs...>&& old_tuple, NewArg&& new_arg) {
        return std::apply([&arg = new_arg]<class... InnerArgs>(InnerArgs&&... args) {
            return std::make_tuple(std::forward<InnerArgs>(args)..., arg);
        }, old_tuple);
    }

    template<class R, class... Args>
    struct typed_call_builder<R, Args...> final : lv::fn_call_builder {
        typed_call_builder()
            requires(sizeof...(Args) == 0)
             : args{} { }

        explicit typed_call_builder(const std::tuple<std::decay_t<Args>...>& args)
               : args(args) { }

        template<class T>
        [[nodiscard]] std::unique_ptr<lv::fn_call_builder>
        add_typed_arg(T&& obj) {
            return std::make_unique<typed_call_builder<R, Args..., T>>(append(std::move(args), std::forward<T>(obj)));
        }

        [[nodiscard]] std::unique_ptr<fn_call_builder>
        add_arg(const lv::linda_value& value) override;

        [[nodiscard]] lv::linda_value
        finalize(std::string_view function_name) const override {
            auto* fn = ldb::load_symbol<R (*)(Args...)>(function_name);
            return std::apply(fn, args);
        }

        std::tuple<std::decay_t<Args>...> args;
    };

    template<>
    struct typed_call_builder<> final : lv::fn_call_builder {
        typed_call_builder() = default;

        template<class T>
        [[nodiscard]] std::unique_ptr<lv::fn_call_builder>
        add_typed_arg(T&& obj) const {
            std::ignore = std::forward<T>(obj);
            return std::make_unique<typed_call_builder<T>>();
        }

        [[nodiscard]] std::unique_ptr<fn_call_builder>
        add_arg(const lv::linda_value& value) override;

        [[nodiscard]] lv::linda_value
        finalize(std::string_view function_name) const override {
            std::ignore = function_name;
            throw std::runtime_error("finalize called on incomplete function call object: at least the result type must be specified");
        }
    };

    std::unique_ptr<lv::fn_call_builder>
    typed_call_builder<>::add_arg(const lv::linda_value& value) {
        const typed_builder_building_visitor<> visitor(*this);
        return std::visit(visitor, value);
    }

    template<class R, class... Args>
    std::unique_ptr<lv::fn_call_builder>
    typed_call_builder<R, Args...>::add_arg(const lv::linda_value& value) {
        const typed_builder_building_visitor<R, Args...> visitor(*this);
        return std::visit(visitor, value);
    }
}

std::unique_ptr<ldb::lv::fn_call_builder>
ldb::lv::get_call_builder() {
    return std::make_unique<typed_call_builder<>>();
}
