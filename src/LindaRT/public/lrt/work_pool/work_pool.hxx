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
 * src/LindaRT/public/lrt/work_pool/work_pool --
 *   
 */
#ifndef LINDADB_WORK_POOL_HXX
#define LINDADB_WORK_POOL_HXX

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <latch>
#include <span>
#include <thread>
#include <vector>

#include <lrt/work_pool/work.hxx>
#include <lrt/work_pool/work_if.hxx>
#include <lrt/work_pool/work_queue.hxx>
// TEMPORARY
#include <lrt/mpi_thread_context.hxx>

#include <mpi.h>

namespace lrt {
    template<std::size_t, class...>
    struct work_pool;

    template<class... WorkerContext,
             work_if<WorkerContext...> Work,
             std::size_t PoolSize>
    struct work_pool<PoolSize, Work, WorkerContext...> {
        using value_type = Work;
        using queue_type = work_queue<value_type, WorkerContext...>;
        using context_builder_type = std::function<std::tuple<WorkerContext...>()>;

        explicit work_pool(const context_builder_type& context_builder,
                           std::size_t pool_size = std::thread::hardware_concurrency())
             : _storage(context_builder, pool_size, _queue) { }

        void
        enqueue(value_type&& work) {
            LDBT_ZONE_A;
            int rank{};
            //            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            _queue.enqueue(std::move(work));
        }

        void
        terminate() {
            LDBT_ZONE_A;
            _queue.terminate();
        }

        ~work_pool() {
            LDBT_ZONE_A;
            _queue.await_terminated();
            std::ranges::for_each(threads(), [](auto& t) { t.join(); });
        }

    private:
        struct worker_thread_job {
            void
            operator()(std::tuple<WorkerContext...>** ctx_pointer,
                       const context_builder_type* builder,
                       queue_type* work_queue,
                       std::latch* latch) const {
                LDBT_ZONE_A;
                std::tuple<WorkerContext...> thread_context = (*builder)();
                *ctx_pointer = &thread_context;
                if constexpr (sizeof...(WorkerContext) > 0) {
                    mpi_thread_context::set_current(std::get<0>(thread_context));
                }

                try {
                    latch->count_down();
                    work_loop(thread_context, *work_queue);
                } catch (work_queue_terminated_exception) {
                    return;
                }
            }

        private:
            void
            work_loop(std::tuple<WorkerContext...>& thread_context,
                      queue_type& work_queue) const {
                LDBT_ZONE("worker loop");
                for (;;) {
                    auto work = work_queue.dequeue();
                    std::apply([&work]<class... CallCtx>(CallCtx&&... context) {
                        work.perform(context...);
                    },
                               thread_context);
                }
            }
        };

        template<std::size_t Size>
        struct pool_storage {
            pool_storage(const context_builder_type& builder,
                         std::size_t size,
                         queue_type& work_queue) {
                std::ignore = size;
                std::latch start_latch(static_cast<std::ptrdiff_t>(Size) + 1);
                std::ranges::generate(_threads,
                                      [n = 0ull, this, &start_latch, &builder, &work_queue]() mutable {
                                          return std::thread(worker_thread_job(), &_context_pointers[n++], &builder, &work_queue, &start_latch);
                                      });
                start_latch.arrive_and_wait();
            }

            std::array<std::tuple<WorkerContext...>*, Size> _context_pointers{};
            std::array<std::thread, Size> _threads{};
        };
        template<>
        struct pool_storage<std::dynamic_extent> {
            pool_storage(const context_builder_type& builder,
                         std::size_t size,
                         queue_type& work_queue)
                 : _context_pointers(size),
                   _threads(size) {
                std::latch start_latch(static_cast<std::ptrdiff_t>(size) + 1);
                std::ranges::generate(_threads,
                                      [n = 0ull, this, &start_latch, &builder, &work_queue]() mutable {
                                          return std::thread(worker_thread_job(), &_context_pointers[n++], &builder, &work_queue, &start_latch);
                                      });
                start_latch.arrive_and_wait();
            }

            std::vector<std::tuple<WorkerContext...>*> _context_pointers;
            std::vector<std::thread> _threads;
        };

        queue_type _queue{};
        pool_storage<PoolSize> _storage;

        std::span<std::thread>
        threads() noexcept {
            return std::span(_storage._threads.begin(),
                             _storage._threads.end());
        }
    };

    work_pool() -> work_pool<std::dynamic_extent, work<>>;
}

#endif
