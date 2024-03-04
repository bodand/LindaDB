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
 * src/LindaRT/public/lrt/work_pool/work_queue --
 *   
 */
#ifndef LINDADB_WORK_QUEUE_HXX
#define LINDADB_WORK_QUEUE_HXX

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>

#include <ldb/common.hxx>
#include <lrt/work_pool/work.hxx>
#include <lrt/work_pool/work_if.hxx>

#include <mpi.h>


namespace lrt {
    struct work_queue_terminated_exception : std::exception {
        [[nodiscard]] const char*
        what() const override {
            return "_work_queue has been terminated";
        }
    };

    template<work_if Work = class work>
    struct work_queue {
        using value_type = Work;

        void
        enqueue(value_type&& work) {
            assert_that(!_terminated.test(), "terminated work queue used");
            std::unique_lock lck(_mtx);
            _queue.emplace(std::move(work));
            _cv.notify_one();
        }

        value_type
        dequeue() {
            if (_terminated.test()) throw work_queue_terminated_exception{};

            std::unique_lock lck(_mtx);
            if (_queue.empty()) _cv.wait(lck, [this] { return !_queue.empty() || _terminated.test(); });
            if (_terminated.test()) throw work_queue_terminated_exception{};

            assert_that(!_queue.empty(), "empty queue popped");
            auto result = std::move(_queue.front());
            _queue.pop();
            return std::move(result);
        }

        void
        terminate() {
            if (!_terminated.test_and_set()) {
                _cv.notify_all();
            }
            _terminated.notify_all();
        }

        void
        await_terminated() {
            if (!_terminated.test()) _terminated.wait(true);
        }

    private:
        std::atomic_flag _terminated = ATOMIC_FLAG_INIT;
        std::queue<value_type> _queue{};
        std::mutex _mtx{};
        std::condition_variable _cv{};
    };

    work_queue() -> work_queue<>;
}

#endif
