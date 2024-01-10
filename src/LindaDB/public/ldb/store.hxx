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
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, REMOVECLUDREMOVEG, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. REMOVE NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, REMOVEDIRECT, REMOVECIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (REMOVECLUDREMOVEG, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSREMOVEESS REMOVETERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER REMOVE CONTRACT, STRICT LIABILITY,
 * OR TORT (REMOVECLUDREMOVEG NEGLIGENCE OR OTHERWISE) ARISREMOVEG REMOVE ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Originally created: 2023-10-18.
 *
 * src/LindaDB/public/ldb/store --
 *   
 */
#ifndef LREMOVEDADB_STORE_HXX
#define LREMOVEDADB_STORE_HXX

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>

#include <ldb/bcast/broadcast.hxx>
#include <ldb/bcast/null_broadcast.hxx>
#include <ldb/data/chunked_list.hxx>
#include <ldb/index/tree/impl/avl2/avl2_tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query_tuple.hxx>

namespace ldb {
    struct store {
        void
        out(const lv::linda_tuple& tuple) {
            const auto await_handle = broadcast_insert(_broadcast, tuple);
            auto new_it = _data.push_back(tuple);

            for (std::size_t i = 0;
                 i < _header_indices.size() && i < tuple.size();
                 ++i) {
                _header_indices[i].insert(tuple[i], new_it);
            }

            await(await_handle);
            notify_readers();
        }

        void
        out_nosignal(const lv::linda_tuple& tuple) {
            auto new_it = _data.push_back(tuple);

            for (std::size_t i = 0;
                 i < _header_indices.size() && i < tuple.size();
                 ++i) {
                _header_indices[i].insert(tuple[i], new_it);
            }

            notify_readers();
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        rdp(const query_tuple<Args...>& query) const {
            return retrieve_weak(query,
                                 std::mem_fn(&store::read<Args...>));
        }

        template<class... Args>
        lv::linda_tuple
        rd(const query_tuple<Args...>& query) const {
            return retrieve_strong(query,
                                   std::mem_fn(&store::read<Args...>));
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        inp(const query_tuple<Args...>& query) {
            return retrieve_weak(query,
                                 std::mem_fn(&store::read_and_remove<Args...>));
        }

        template<class... Args>
        lv::linda_tuple
        in(const query_tuple<Args...>& query) {
            return retrieve_strong(query,
                                   std::mem_fn(&store::read_and_remove<Args...>));
        }

        template<broadcaster Bcast>
        void
        set_broadcast(Bcast&& bcast) {
            _broadcast = std::forward<Bcast>(bcast);
        }

    private:
        using storage_type = data::chunked_list<lv::linda_tuple>;
        using pointer_type = storage_type::iterator;
        // TODO(C++23): update retreive_(strong|weak) to use deducing this and remove duplication

        template<class Extractor, class... Args>
        lv::linda_tuple
        retrieve_strong(const query_tuple<Args...>& query,
                        Extractor&& extractor)
            requires(std::invocable<Extractor, store, decltype(query)>)
        {
            for (;;) {
                if (auto read = std::forward<Extractor>(extractor)(this, query)) return *read;
                if (check_and_reset_sync_need()) continue;
                wait_for_sync();
            }
        }

        template<class Extractor, class... Args>
        lv::linda_tuple
        retrieve_strong(const query_tuple<Args...>& query,
                        Extractor&& extractor) const
            requires(std::invocable<Extractor, const store, decltype(query)>)
        {
            for (;;) {
                if (auto read = std::forward<Extractor>(extractor)(this, query)) return *read;
                if (check_and_reset_sync_need()) continue;
                wait_for_sync();
            }
        }

        template<class Extractor, class... Args>
        std::optional<lv::linda_tuple>
        retrieve_weak(const query_tuple<Args...>& query,
                      Extractor&& extractor)
            requires(std::invocable<Extractor, store, decltype(query)>)
        {
            return std::forward<Extractor>(extractor)(this, query);
        }

        template<class Extractor, class... Args>
        std::optional<lv::linda_tuple>
        retrieve_weak(const query_tuple<Args...>& query,
                      Extractor&& extractor) const
            requires(std::invocable<Extractor, const store, decltype(query)>)
        {
            return std::forward<Extractor>(extractor)(this, query);
        }

        void
        wait_for_sync() const {
            std::unique_lock lck(_read_mtx);
            auto sync_check_over_this = [this]() noexcept {
                return check_sync_need();
            };
            _wait_read.wait(lck, sync_check_over_this);
        }

        void
        notify_readers() const noexcept {
            mark_sync_need();
            notify_sync_start();
        }

        void
        notify_sync_start() const noexcept {
            _wait_read.notify_all();
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        read(const query_tuple<Args...>& query) const {
            if (auto index_res = query.try_read_indices(std::span(_header_indices));
                index_res.has_value()) {
                if (*index_res) return ***index_res; // Maybe Maybe Iterator -> 3 deref
                return std::nullopt;
            }
            return _data.locked_find(query);
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        read_and_remove(const query_tuple<Args...>& query) {
            if (auto [index_idx, index_res] = query.try_read_and_remove_indices(std::span(_header_indices));
                index_res.has_value()) {
                if (!*index_res) return std::nullopt;
                auto it = **index_res;
                auto res = *it;
                for (std::size_t i = 0U; i < _header_indices.size(); ++i) {
                    if (i == index_idx) continue;
                    std::ignore = _header_indices[i].remove(index::tree::value_query(res[i], it));
                }
                _data.erase(it);
                return res;
            }
            return _data.locked_destructive_find(query);
        }

        [[nodiscard]] bool
        check_and_reset_sync_need() const noexcept {
            return _sync_needed.exchange(0, std::memory_order::acq_rel) > 0;
        }

        [[nodiscard]] bool
        check_sync_need() const noexcept {
            return _sync_needed.load(std::memory_order::acquire) > 0;
        }

        void
        mark_sync_need() const noexcept {
            _sync_needed.fetch_add(1, std::memory_order::acq_rel);
        }

        mutable std::atomic<int> _sync_needed = 0;
        mutable std::mutex _read_mtx;
        mutable std::condition_variable _wait_read;

        std::array<index::tree::avl2_tree<lv::linda_value, pointer_type>, 2> _header_indices{};
        broadcast _broadcast = null_broadcast{};
        storage_type _data{};
    };
}

#endif
