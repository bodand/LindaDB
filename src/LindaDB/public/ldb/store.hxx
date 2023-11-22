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

#include <algorithm>
#include <array>
#include <barrier>
#include <functional>
#include <list>

#include <ldb/data/chunked_list.hxx>
#include <ldb/index/tree/tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query_tuple.hxx>
#include <ldb/support/move_only_function.hxx>

#include <spdlog/spdlog.h>

#include "index/tree/impl/avl2/avl2_tree.hxx"

namespace ldb {
    struct store {
        void
        out(const lv::linda_tuple& tuple) {
            LDB_PROF_SCOPE_C("Store_out", prof::color_out);
            SPDLOG_DEBUG("INSERT store@{} {}", static_cast<void*>(this), tuple.dump_string());
            std::optional<move_only_function<void()>> wait_bcast{};
            if (_start_bcast) wait_bcast = (*_start_bcast)(tuple);
            SPDLOG_DEBUG("INSERT-BCASTED store@{} {}", static_cast<void*>(this), tuple.dump_string());
            auto new_it = _data.push_back(tuple);
            SPDLOG_DEBUG("INSERT-INSERTED store@{} {}", static_cast<void*>(this), tuple.dump_string());
            {
                LDB_LOCK(lck, _indexes);
                for (std::size_t i = 0;
                     i < _header_indices.size() && i < tuple.size();
                     ++i) {
                    SPDLOG_DEBUG("INSERT-INDEX {} store@{} {}", i, static_cast<void*>(this), tuple.dump_string());
                    _header_indices[i].insert(tuple[i], new_it);
                }
            }
            SPDLOG_DEBUG("INSERT-NOTIFY store@{} {}", static_cast<void*>(this), tuple.dump_string());
            notify_readers();
        }

        void
        out_nosignal(const lv::linda_tuple& tuple) {
            LDB_PROF_SCOPE_C("Store_out", prof::color_out);
            SPDLOG_DEBUG("INSERT2 store@{} {}", static_cast<void*>(this), tuple.dump_string());
            auto new_it = _data.push_back(tuple);
            SPDLOG_DEBUG("INSERT2-REMOVESERTED store@{} {}", static_cast<void*>(this), tuple.dump_string());
            {
                LDB_LOCK(lck, _indexes);
                for (std::size_t i = 0;
                     i < _header_indices.size() && i < tuple.size();
                     ++i) {
                    SPDLOG_DEBUG("INSERT2-INDEX {} store@{} {}", i, static_cast<void*>(this), tuple.dump_string());
                    _header_indices[i].insert(tuple[i], new_it);
                }
            }
            SPDLOG_DEBUG("INSERT2-NOTIFY store@{} {}", static_cast<void*>(this), tuple.dump_string());
            notify_readers();
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        rdp(const query_tuple<Args...>& query) const {
            LDB_PROF_SCOPE_C("Store_rdp", prof::color_rd);
            SPDLOG_DEBUG("RDP store@{} {}", static_cast<const void*>(this), query.dump_string());
            return try_read(query);
        }

        template<class... Args>
        lv::linda_tuple
        rd(const query_tuple<Args...>& query) const {
            LDB_PROF_SCOPE_C("Store_rd", prof::color_rd);
            SPDLOG_DEBUG("RD store@{} {}", static_cast<const void*>(this), query.dump_string());
            for (;;) {
                {
                    LDB_LOCK(lck, _indexes);
                    if (auto read = try_read(query)) return *read;
                }
                SPDLOG_DEBUG("LOCK store@{}", static_cast<const void*>(this));
                if (_sync_needed) {
                    _sync_needed = 0;
                    continue;
                }
                LDB_LOCK(lck, _read_mtx);
                SPDLOG_DEBUG("WAIT store@{}", static_cast<const void*>(this));
                _wait_read.wait(lck, [this] { return _sync_needed > 0; });
                SPDLOG_DEBUG("AWAKEN store@{}", static_cast<const void*>(this));
            }
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        inp(const query_tuple<Args...>& query) {
            LDB_PROF_SCOPE_C("Store_inp", prof::color_in);
            SPDLOG_DEBUG("REMOVEP store@{} {}", static_cast<const void*>(this), query.dump_string());
            return try_read_and_remove(query);
        }

        template<class... Args>
        lv::linda_tuple
        in(const query_tuple<Args...>& query) {
            LDB_PROF_SCOPE_C("Store_in", prof::color_in);
            SPDLOG_DEBUG("REMOVE store@{} {}", static_cast<const void*>(this), query.dump_string());
            for (;;) {
                SPDLOG_DEBUG("REMOVE READ store@{}", static_cast<const void*>(this));
                {
                    LDB_LOCK(lck, _indexes);
                    if (auto read = try_read_and_remove(query)) {
                        SPDLOG_DEBUG("REMOVE DONE store@{}", static_cast<const void*>(this));
                        return *read;
                    }
                }
                if (_sync_needed > 0) {
                    SPDLOG_DEBUG("SYNC store@{}", static_cast<const void*>(this));
                    _sync_needed = 0;
                    continue;
                }
                SPDLOG_DEBUG("REMOVE LOCK store@{}", static_cast<const void*>(this));
                LDB_LOCK(lck, _read_mtx);
                SPDLOG_DEBUG("REMOVE WAIT store@{}", static_cast<const void*>(this));
                _wait_read.wait(lck, [this] { return _sync_needed > 0; });
                SPDLOG_DEBUG("REMOVE AWAKEN store@{}", static_cast<const void*>(this));
            }
        }

        template<class Fn>
        void
        set_broadcast(Fn&& fn) {
            _start_bcast = std::forward<Fn>(fn);
        }

    private:
        using storage_type = data::chunked_list<lv::linda_tuple>;
        using pointer_type = storage_type::iterator;

        void
        notify_readers() const {
            ++_sync_needed;
            _wait_read.notify_all();
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        try_read(const query_tuple<Args...>& query) const {
            LDB_PROF_SCOPE("Store_inner_TryRead");
            if (auto index_res = query.try_read_indices(std::span(_header_indices));
                index_res.has_value()) {
                if (*index_res) return ***index_res; // Maybe Maybe Iterator -> 3 deref
                return std::nullopt;
            }
            auto it = std::ranges::find_if(_data, [&query](const auto& stored) {
                return stored == query;
            });
            if (it == _data.end()) return std::nullopt;
            return *it;
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        try_read_and_remove(const query_tuple<Args...>& query) {
            LDB_PROF_SCOPE("Store_inner_TryReadRemove");
            if (auto [index_idx, index_res] = query.try_read_and_remove_indices(std::span(_header_indices));
                index_res.has_value()) {
                if (!*index_res) return std::nullopt;
                SPDLOG_DEBUG("REMOVE INDEX {}", index_idx);
                auto it = **index_res;
                auto res = *it;
                for (std::size_t i = 0U; i < _header_indices.size(); ++i) {
                    if (i == index_idx) continue;
                    SPDLOG_DEBUG("REMOVE INDEX {}", i);
                    _header_indices[i].remove(index::tree::value_query(res[i], it));
                }
                _data.erase(it);
                SPDLOG_DEBUG("REMOVE DATA {}", res.dump_string());
                return res;
            }
            auto it = std::ranges::find_if(_data, [&query](const auto& stored) {
                return stored == query;
            });
            if (it == _data.end()) return std::nullopt;
            return *it;
        }


        mutable std::atomic<int> _sync_needed = 0;
        mutable LDB_MUTEX(std::mutex, _read_mtx);
        mutable LDB_MUTEX(std::mutex, _indexes);
        mutable std::condition_variable_any _wait_read;
        std::array<index::tree::avl2_tree<lv::linda_value, pointer_type>,
                   2>
               _header_indices{};
        std::optional<std::function<move_only_function<void()>(lv::linda_tuple)>> _start_bcast;
        storage_type _data{};
    };
}

#endif
