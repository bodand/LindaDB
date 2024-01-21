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
#include <ldb/lv/linda_tuple.hxx>

namespace ldb {
    class broadcast_awaitable final {
        struct awaitable_concept {
            virtual void
            do_await() = 0;

            virtual ~awaitable_concept() = default;
        };

        template<awaitable Impl>
        struct awaitable_model final : awaitable_concept {
            using impl_type = Impl;

            template<awaitable T = impl_type>
            explicit awaitable_model(T&& init) /* todo noexcept in some cases... */
                requires(!std::same_as<T, awaitable_model>)
                 : value(std::forward<T>(init)) { }

            void
            do_await() override {
                await(value);
            }

            impl_type value;
        };

        std::unique_ptr<awaitable_concept> _impl = nullptr;

        friend void
        await(const broadcast_awaitable& bcast_await_handle) {
            if (bcast_await_handle._impl) bcast_await_handle._impl->do_await();
        }

    public:
        broadcast_awaitable() = default;
        ~broadcast_awaitable() = default;

        template<awaitable Impl>
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

    class broadcast final {
        struct broadcast_concept {
            virtual broadcast_awaitable
            do_broadcast_insert(const lv::linda_tuple& tuple) = 0;
            virtual broadcast_awaitable
            do_broadcast_delete(const lv::linda_tuple& tuple) = 0;

            virtual ~broadcast_concept() = default;
        };

        template<broadcaster Impl>
        struct broadcast_model final : broadcast_concept {
            using impl_type = Impl;

            template<broadcaster T = impl_type>
            explicit broadcast_model(T&& init) /* todo noexcept in some cases... */
                requires(!std::same_as<T, broadcast_model>)
                 : bcast(std::forward<T>(init)) { }

            broadcast_awaitable
            do_broadcast_insert(const lv::linda_tuple& tuple) override {
                return broadcast_insert(bcast, tuple);
            }
            broadcast_awaitable
            do_broadcast_delete(const lv::linda_tuple& tuple) override {
                return broadcast_delete(bcast, tuple);
            }

            impl_type bcast;
        };

        std::unique_ptr<broadcast_concept> _impl = nullptr;

        friend broadcast_awaitable
        broadcast_insert(const broadcast& bcast,
                         const lv::linda_tuple& tuple) {
            if (!bcast._impl) return {};
            return bcast._impl->do_broadcast_insert(tuple);
        }

        friend broadcast_awaitable
        broadcast_delete(const broadcast& bcast,
                         const lv::linda_tuple& tuple) {
            if (!bcast._impl) return {};
            return bcast._impl->do_broadcast_delete(tuple);
        }

    public:
        broadcast() = default;
        ~broadcast() = default;

        template<broadcaster Impl>
        explicit(false) broadcast(Impl value)
             : _impl(std::make_unique<broadcast_model<Impl>>(std::move(value))) { }

        broadcast(const broadcast& other) = delete;
        broadcast&
        operator=(const broadcast& other) = delete;

        broadcast(broadcast&& other) noexcept
             : _impl(std::move(other._impl)) { }
        broadcast&
        operator=(broadcast&& other) noexcept {
            if (this == &other) return *this;
            _impl = std::move(other._impl);
            return *this;
        }
    };
}

#endif
