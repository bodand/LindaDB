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
 * test/LindaRT/work_queue --
 *   
 */

#include <array>
#include <concepts>
#include <ostream>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <lrt/work_pool/work_queue.hxx>

namespace {
    struct test_work {
        int payload;

        void
        perform() { }

        friend std::ostream&
        operator<<(std::ostream& os, const test_work& /*ignored*/) {
            return os;
        }
    };
}

TEST_CASE("_work_queue is default constructible") {
    STATIC_CHECK(std::constructible_from<lrt::work_queue<test_work>>);
}

TEST_CASE("_work_queue is not copyable/movable") {
    STATIC_CHECK_FALSE(std::copyable<lrt::work_queue<>>);
    STATIC_CHECK_FALSE(std::movable<lrt::work_queue<>>);
}

TEST_CASE("serial io of _work_queue works") {
    lrt::work_queue<test_work> work_queue;

    work_queue.enqueue(test_work{42});
    auto work = work_queue.dequeue();
    CHECK(work.payload == 42);
}

TEST_CASE("parallel io of _work_queue works") {
    lrt::work_queue<test_work> work_queue;
    std::atomic<bool> results = true;

    {
        auto writer_thread = [&work_queue]() {
            for (int i = 0; i < 500'000; ++i) {
                work_queue.enqueue(test_work{42});
            }
        };
        auto reader_thread = [&work_queue, &results]() {
            for (int i = 0; i < 500'000; ++i) {
                auto work = work_queue.dequeue();
                results = results && work.payload == 42;
            }
        };

        const std::array holder{
               std::jthread(writer_thread),
               std::jthread(writer_thread),
               std::jthread(writer_thread),
               std::jthread(writer_thread),
               std::jthread(reader_thread),
               std::jthread(reader_thread),
               std::jthread(reader_thread),
               std::jthread(reader_thread),
        };
    }
    CHECK(results);
}
