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
 * Originally created: 2023-10-18.
 *
 * src/LindaDB/public/ldb/store --
 *   
 */
#ifndef LINDADB_STORE_HXX
#define LINDADB_STORE_HXX

#include <algorithm>
#include <array>
#include <list>

#include <ldb/data/chunked_list.hxx>
#include <ldb/index/tree/tree.hxx>
#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query_tuple.hxx>

namespace ldb {
    struct store {
        void
        out(const lv::linda_tuple& tuple) {
            LDB_PROF_SCOPE_C("Store_out", prof::color_out);
            auto new_it = _data.push_back(tuple);
            for (std::size_t i = 0;
                 i < _header_indices.size() && i < tuple.size();
                 ++i) {
                _header_indices[i].insert(tuple[i], new_it);
            }
            _wait.notify_all();
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        rdp(const query_tuple<Args...>& query) {
            LDB_PROF_SCOPE_C("Store_rdp", prof::color_rd);
            return try_read(query);
        }

        template<class... Args>
        lv::linda_tuple
        rd(const query_tuple<Args...>& query) {
            LDB_PROF_SCOPE_C("Store_rd", prof::color_rd);
            for (;;) {
                if (auto read = try_read(query)) return *read;
                LDB_LOCK(lck, _wait_mtx);
                _wait.wait(lck);
            }
        }

        template<class... Args>
        std::optional<lv::linda_tuple>
        inp(const query_tuple<Args...>& query) {
            LDB_PROF_SCOPE_C("Store_inp", prof::color_in);
            return try_read_and_remove(query);
        }

        template<class... Args>
        lv::linda_tuple
        in(const query_tuple<Args...>& query) {
            LDB_PROF_SCOPE_C("Store_in", prof::color_in);
            for (;;) {
                if (auto read = try_read_and_remove(query)) return *read;
                LDB_LOCK(lck, _wait_mtx);
                _wait.wait(lck);
            }
        }

    private:
        using storage_type = data::chunked_list<lv::linda_tuple>;
        using pointer_type = storage_type::iterator;

        template<class... Args>
        std::optional<lv::linda_tuple>
        try_read(const query_tuple<Args...>& query) {
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
                auto it = **index_res;
                auto res = *it;
                for (std::size_t i = 0U;
                     auto& idx : _header_indices) {
                    if (i == index_idx) continue;
                    idx.remove(index::tree::value_query(res[i], it));
                    ++i;
                }
                _data.erase(it);
                return res;
            }
            auto it = std::ranges::find_if(_data, [&query](const auto& stored) {
                return stored == query;
            });
            if (it == _data.end()) return std::nullopt;
            return *it;
        }

        LDB_MUTEX(std::mutex, _wait_mtx);
        std::condition_variable_any _wait;
        std::array<index::tree::tree<lv::linda_value, pointer_type>, 2> _header_indices{};
        storage_type _data{};
    };
}

#endif
