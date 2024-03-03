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
 * src/LindaRT/public/lrt/balancer/balancer --
 *   
 */
#ifndef LINDADB_BALANCER_HXX
#define LINDADB_BALANCER_HXX

#include <memory>

#include <ldb/lv/linda_tuple.hxx>
#include <lrt/balancer/balancer_if.hxx>

namespace lrt {
    class balancer final {
        struct balancer_concept {
            balancer_concept(const balancer_concept& cp) = delete;
            balancer_concept&
            operator=(const balancer_concept& cp) = delete;

            balancer_concept(balancer_concept&& mv) noexcept = delete;
            balancer_concept&
            operator=(balancer_concept&& mv) noexcept = delete;

            virtual ~balancer_concept() noexcept = default;

            [[nodiscard]] virtual std::unique_ptr<balancer_concept>
            clone() const = 0;

            [[nodiscard]] virtual int
            do_send_to_rank(const ldb::lv::linda_tuple& tuple) = 0;

        protected:
            balancer_concept() noexcept = default;
        };

        template<balancer_if Balancer>
        struct balancer_model final : balancer_concept {
            explicit balancer_model(const Balancer& balancer_impl)
                 : balancer_impl(balancer_impl) { }

            std::unique_ptr<balancer_concept>
            clone() const override {
                return std::make_unique<balancer_model>(balancer_impl);
            }

            int
            do_send_to_rank(const ldb::lv::linda_tuple& tuple) override {
                return balancer_impl.send_to_rank(tuple);
            }

            Balancer balancer_impl;
        };

        std::unique_ptr<balancer_concept> _impl;

    public:
        template<class Balancer>
        explicit(false) balancer(Balancer&& impl)
            requires(!std::same_as<balancer, Balancer>)
             : _impl(std::make_unique<balancer_model<Balancer>>(std::forward<Balancer>(impl))) { }

        balancer(const balancer& cp)
             : _impl(cp._impl->clone()) { }
        balancer&
        operator=(const balancer& cp) {
            if (&cp != this) _impl = cp._impl->clone();
            return *this;
        }

        balancer(balancer&& mv) noexcept = default;
        balancer&
        operator=(balancer&& mv) noexcept = default;

        ~balancer() noexcept = default;

        [[nodiscard]] int
        send_to_rank(const ldb::lv::linda_tuple& tuple) {
            return _impl->do_send_to_rank(tuple);
        }
    };
}

#endif
