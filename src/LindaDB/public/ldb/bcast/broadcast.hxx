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
 * Originally created: 2024-01-10.
 *
 * src/LindaDB/public/ldb/bcast/broadcast --
 *   A type erased broadcast type that can store a singular broadcaster
 *   implementation.
 */
#ifndef LINDADB_BROADCAST_HXX
#define LINDADB_BROADCAST_HXX

#include <concepts>
#include <memory>
#include <utility>

#include <ldb/bcast/broadcaster.hxx>
#include <ldb/bcast/null_broadcast.hxx>
#include <ldb/lv/linda_tuple.hxx>

namespace ldb {
    template<class R = void>
    class broadcast_awaitable final {
        struct awaitable_concept {
            virtual R
            do_await() = 0;

            virtual ~awaitable_concept() = default;
        };

        template<await_if<R> Impl>
        struct awaitable_model final : awaitable_concept {
            using impl_type = Impl;

            template<await_if<R> T = impl_type>
            explicit awaitable_model(T&& init) /* todo noexcept in some cases... */
                requires(!std::same_as<T, awaitable_model>)
                 : value(std::forward<T>(init)) { }

            R
            do_await() override {
                return await(value);
            }

            impl_type value;
        };

        std::unique_ptr<awaitable_concept> _impl = nullptr;

        friend R
        await(const broadcast_awaitable& bcast_await_handle) {
            if (bcast_await_handle._impl) return bcast_await_handle._impl->do_await();
            return R{};
        }

        friend void
        await(const broadcast_awaitable& bcast_await_handle)
            requires(std::same_as<R, void>)
        {
            if (bcast_await_handle._impl) bcast_await_handle._impl->do_await();
        }

    public:
        broadcast_awaitable() = default;
        ~broadcast_awaitable() = default;

        template<await_if<R> Impl>
        explicit(false) broadcast_awaitable(Impl value)
             : _impl(std::make_unique<awaitable_model<Impl>>(std::move(value))) { }

        broadcast_awaitable(const broadcast_awaitable& other) = delete;
        broadcast_awaitable&
        operator=(const broadcast_awaitable& other) = delete;

        broadcast_awaitable(broadcast_awaitable&& other) noexcept
             : _impl(std::move(other._impl)) {
            other._impl.reset();
        }
        broadcast_awaitable&
        operator=(broadcast_awaitable&& other) noexcept {
            if (this == &other) return *this;
            _impl = std::move(other._impl);
            return *this;
        }
    };

    template<class RTerminate,
             class REval,
             class RInsert,
             class RDelete,
             class TContext>
    class broadcast final {
        struct broadcast_concept {
            [[nodiscard]] virtual broadcast_awaitable<RInsert>
            do_broadcast_insert(const lv::linda_tuple& tuple) = 0;

            [[nodiscard]] virtual broadcast_awaitable<RDelete>
            do_broadcast_delete(const lv::linda_tuple& tuple) = 0;

            [[nodiscard]] virtual std::vector<broadcast_msg<TContext>>
            do_broadcast_recv() = 0;

            [[nodiscard]] virtual broadcast_awaitable<RTerminate>
            do_broadcast_terminate() = 0;

            [[nodiscard]] virtual broadcast_awaitable<REval>
            do_send_eval(int to, const lv::linda_tuple& tuple) = 0;

            [[nodiscard]] virtual std::unique_ptr<broadcast_concept>
            clone() = 0;

            virtual ~broadcast_concept() = default;
        };

        template<broadcast_if<RTerminate, REval, RInsert, RDelete, TContext> Impl>
        struct broadcast_model final : broadcast_concept {
            using impl_type = Impl;

            template<class T = impl_type>
            explicit broadcast_model(T&& init) /* todo noexcept in some cases... */
                requires(!std::same_as<T, broadcast_model>
                         && broadcast_if<std::remove_cvref_t<T>, RTerminate, REval, RInsert, RDelete, TContext>)
                 : bcast(std::forward<T>(init)) { }

            [[nodiscard]] broadcast_awaitable<RInsert>
            do_broadcast_insert(const lv::linda_tuple& tuple) override {
                return broadcast_insert(bcast, tuple);
            }

            [[nodiscard]] broadcast_awaitable<RDelete>
            do_broadcast_delete(const lv::linda_tuple& tuple) override {
                return broadcast_delete(bcast, tuple);
            }

            [[nodiscard]] std::vector<broadcast_msg<TContext>>
            do_broadcast_recv() override {
                return broadcast_recv(bcast);
            }

            [[nodiscard]] std::unique_ptr<broadcast_concept>
            clone() override {
                return std::make_unique<broadcast_model>(bcast);
            }

            broadcast_awaitable<RTerminate>
            do_broadcast_terminate() override {
                return broadcast_terminate(bcast);
            }

            broadcast_awaitable<REval>
            do_send_eval(int to, const lv::linda_tuple& tuple) override {
                return send_eval(bcast, to, tuple);
            }

            impl_type bcast;
        };

        std::unique_ptr<broadcast_concept> _impl = nullptr;

        [[nodiscard]] friend broadcast_awaitable<RInsert>
        broadcast_insert(const broadcast& bcast,
                         const lv::linda_tuple& tuple) {
            if (!bcast._impl) return null_awaiter<RInsert>{};
            return bcast._impl->do_broadcast_insert(tuple);
        }

        [[nodiscard]] friend broadcast_awaitable<RDelete>
        broadcast_delete(const broadcast& bcast,
                         const lv::linda_tuple& tuple) {
            if (!bcast._impl) return null_awaiter<RDelete>{};
            return bcast._impl->do_broadcast_delete(tuple);
        }

        [[nodiscard]] friend std::vector<broadcast_msg<TContext>>
        broadcast_recv(const broadcast& bcast) {
            if (!bcast._impl) return {};
            return bcast._impl->do_broadcast_recv();
        }

        [[nodiscard]] friend broadcast_awaitable<RTerminate>
        broadcast_terminate(const broadcast& bcast) {
            if (!bcast._impl) return null_awaiter<RTerminate>{};
            return bcast._impl->do_broadcast_terminate();
        }

        [[nodiscard]] friend broadcast_awaitable<REval>
        send_eval(const broadcast& bcast,
                  int to,
                  const lv::linda_tuple& tuple) {
            if (!bcast._impl) return null_awaiter<REval>{};
            return bcast._impl->do_send_eval(to, tuple);
        }

    public:
        broadcast() = default;
        ~broadcast() = default;

        template<broadcast_if<RTerminate, REval, RInsert, RDelete, TContext> Impl>
        explicit(false) broadcast(Impl value)
             : _impl(std::make_unique<broadcast_model<Impl>>(std::move(value))) { }

        broadcast(const broadcast& other)
             : _impl(other._impl->clone()) { }
        broadcast&
        operator=(const broadcast& other) {
            _impl = other._impl->clone();
            return *this;
        }

        broadcast(broadcast&& other) noexcept = default;
        broadcast&
        operator=(broadcast&& other) noexcept = default;
    };
}

#endif
