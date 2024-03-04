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
#include <span>
#include <thread>
#include <vector>

#include <lrt/work_pool/work.hxx>
#include <lrt/work_pool/work_if.hxx>
#include <lrt/work_pool/work_queue.hxx>

#include <mpi.h>

namespace lrt {
    template<work_if Work = class work, std::size_t PoolSize = std::dynamic_extent>
    struct work_pool {
        using value_type = Work;

        explicit work_pool(std::size_t pool_size = std::thread::hardware_concurrency())
             : _storage(pool_size, _queue) { }

        void
        enqueue(value_type&& work) {
            int rank{};
            //            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            std::ofstream("_log.txt", std::ios::app) << rank << ": ENQUEUE POOL: " << work << std::endl;
            _queue.enqueue(std::move(work));
        }

        void
        terminate() {
            _queue.terminate();
        }

        ~work_pool() {
            _queue.await_terminated();
            std::ranges::for_each(threads(), [](auto& t) { t.join(); });
        }

//    private:
        struct worker_thread_job {
            explicit worker_thread_job(work_queue<>& work_queue) : _work_queue(work_queue) { }

            void
            operator()() const {
                try {
                    work_loop();
                } catch (work_queue_terminated_exception) {
                    return;
                }
            }

        private:
            void
            work_loop() const {
                for (;;) {
                    auto work = _work_queue.dequeue();
                    work.perform();
                }
            }

            work_queue<>& _work_queue;
        };

        template<std::size_t Size>
        struct pool_storage {
            pool_storage(std::size_t size, work_queue<>& work_queue)
                 : _threads() {
                std::ignore = size;
                std::ranges::generate(_threads,
                                      [&work_queue]() {
                                          return std::thread{worker_thread_job(work_queue)};
                                      });
            }

            std::array<std::thread, Size> _threads{};
        };
        template<>
        struct pool_storage<std::dynamic_extent> {
            pool_storage(std::size_t size, work_queue<>& work_queue)
                 : _threads(size) {
                std::ranges::generate(_threads,
                                      [&work_queue]() {
                                          return std::thread{worker_thread_job(work_queue)};
                                      });
            }

            std::vector<std::thread> _threads;
        };

        work_queue<> _queue{};
        pool_storage<PoolSize> _storage;

        std::span<std::thread>
        threads() noexcept { return _storage._threads; }
        std::span<std::thread>
        threads() const noexcept { return _storage._threads; }
    };

    work_pool() -> work_pool<>;
}

#endif
