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
 * src/LindaRT/public/lrt/work_pool/work --
 *   
 */
#ifndef LINDADB_WORK_HXX
#define LINDADB_WORK_HXX

#include <fstream>
#include <memory>

#include <lrt/work_pool/work_if.hxx>

#include <mpi.h>

namespace lrt {
    template<class... Context>
    class work final {
        struct work_concept {
            work_concept(const work_concept& cp) = delete;
            work_concept&
            operator=(const work_concept& cp) = delete;

            work_concept(work_concept&& mv) = delete;
            work_concept&
            operator=(work_concept&& mv) = delete;

            virtual ~work_concept() noexcept = default;

            virtual void
            do_perform(const Context&... context) = 0;

            virtual void
            write_to(std::ostream& os) = 0;

        protected:
            work_concept() noexcept = default;
        };

        template<work_if<Context...> Work>
        struct work_model final : work_concept {
            explicit work_model(Work work) : work(work) { }

            void
            do_perform(const Context&... ctx) override {
                int rank;
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                this->work.perform(ctx...);
            }

            void
            write_to(std::ostream& os) override {
                os << work;
            }

            Work work;
        };

        std::unique_ptr<work_concept> _impl;

        friend std::ostream&
        operator<<(std::ostream& os, const work& work) {
            work._impl->write_to(os);
            return os;
        }

    public:
        template<class Work>
        explicit(false) work(Work&& work)
            requires(!std::same_as<std::remove_cvref_t<Work>, class work>)
             : _impl(std::make_unique<work_model<std::remove_cvref_t<Work>>>(std::forward<Work>(work))) { }

        work(const work& cp) = delete;
        work&
        operator=(const work& cp) = delete;

        work(work&& mv) noexcept = default;
        work&
        operator=(work&& mv) noexcept = default;

        ~work() noexcept = default;

        template<class... Args>
        void
        perform(Args&&... args) { _impl->do_perform(std::forward<Args>(args)...); }
    };
}

#endif
