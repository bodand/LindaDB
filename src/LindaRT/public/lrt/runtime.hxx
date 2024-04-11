/* LindaDB project
 *
 * Copyright (c) 2023 András Bodor <bodand@pm.me>
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
 * Originally created: 2023-11-06.
 *
 * src/LindaRT/public/lrt/runtime --
 *   The runtime object for handling the required resources for the LindaRT.
 */
#ifndef LINDADB_RUNTIME_HXX
#define LINDADB_RUNTIME_HXX

#include <atomic>
#include <functional>
#include <span>
#include <thread>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/tuple_query.hxx>
#include <ldb/store.hxx>
#include <lrt/balancer/balancer.hxx>
#include <lrt/balancer/round_robin_balancer.hxx>
#include <lrt/mpi_runtime.hxx>
#include <lrt/mpi_thread_context.hxx>
#include <lrt/work_pool/work.hxx>
#include <lrt/work_pool/work_pool.hxx>

namespace lrt {
    struct runtime {
        runtime(int* argc, char*** argv,
                std::function<balancer(const runtime&)> load_balancer //
                = [](const lrt::runtime& rt) -> lrt::balancer { return lrt::round_robin_balancer(rt.world_size()); });

        runtime(const runtime& cp) = delete;
        runtime(runtime&& mv) noexcept = delete;
        runtime&
        operator=(const runtime& cp) = delete;
        runtime&
        operator=(runtime&& mv) noexcept = delete;

        ~runtime() noexcept;

        void
        eval(const ldb::lv::linda_tuple& call_tuple);

        ldb::store&
        store() noexcept { return _store; }

        const ldb::store&
        store() const noexcept { return _store; }

        [[nodiscard]] int
        rank() const noexcept { return _mpi.rank(); }

        [[nodiscard]] int
        world_size() const noexcept { return _mpi.world_size(); }

        void
        out(const ldb::lv::linda_tuple& tuple) {
            LDBT_ZONE_A;
            if (_mpi.rank() == 0) return (void) _store.insert(tuple);
            remote_insert(tuple);
        }

        template<class... Args>
        void
        in(Args&&... args) {
            LDBT_ZONE_A;
            if (_mpi.rank() == 0) return (void) _store.remove(std::forward<Args>(args)...);
            remote_remove(ldb::make_piecewise_query(_store.indices(),
                                                    std::forward<Args>(args)...));
        }

        template<class... Args>
        bool
        inp(Args&&... args) {
            LDBT_ZONE_A;
            if (_mpi.rank() == 0) return _store.try_remove(std::forward<Args>(args)...).has_value();
            return remote_try_remove(ldb::make_piecewise_query(_store.indices(),
                                                               std::forward<Args>(args)...));
        }

        template<class... Args>
        void
        rd(Args&&... args) {
            LDBT_ZONE_A;
            if (_mpi.rank() == 0) return (void) _store.read(std::forward<Args>(args)...);
            remote_read(ldb::make_piecewise_query(_store.indices(),
                                                  std::forward<Args>(args)...));
        }

        template<class... Args>
        bool
        rdp(Args&&... args) {
            LDBT_ZONE_A;
            if (_mpi.rank() == 0) return _store.try_read(std::forward<Args>(args)...).has_value();
            return remote_try_read(ldb::make_piecewise_query(_store.indices(),
                                                             std::forward<Args>(args)...));
        }

        void
        ack(int to, int ack, std::span<std::byte> data = std::span<std::byte>());

        void
        loop();

    private:
        void
        recv_thread_worker();

        void
        remote_insert(const ldb::lv::linda_tuple& tuple);

        bool
        remote_try_remove(const ldb::tuple_query<ldb::store::index_type>& query);

        void
        remote_remove(const ldb::tuple_query<ldb::store::index_type>& query);

        bool
        remote_try_read(const ldb::tuple_query<ldb::store::index_type>& query);

        void
        remote_read(const ldb::tuple_query<ldb::store::index_type>& query);

        lrt::mpi_runtime _mpi;

        std::atomic_flag _recv_start = ATOMIC_FLAG_INIT;
        std::thread _recv_thr;

        std::optional<balancer> _balancer{std::nullopt};

        lrt::work_pool<std::dynamic_extent, work<>> _work_pool;

        ldb::store _store{};
    };
}

#ifndef LRT_NOINCLUDE_EVAL
#  include <lrt/eval.hxx>
#endif

#endif
