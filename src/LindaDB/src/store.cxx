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
 * Originally created: 2023-10-27.
 *
 * src/LindaDB/src/store --
 */

#include <ldb/bcast/yes_mock_broadcast.hxx>
#include <ldb/store.hxx>

namespace {
    template<class TCommand>
    struct query_result_visitor {
        using continuation_command = std::optional<TCommand>;
        using index_type = std::remove_const_t<typename TCommand::index_type>;

        std::pair<bool, continuation_command>
        operator()(ldb::field_incomparable) const noexcept {
            return std::make_pair(true, continuation_command{});
        }

        std::pair<bool, continuation_command>
        operator()(ldb::field_not_found) const noexcept {
            return std::make_pair(false, continuation_command{});
        }

        std::pair<bool, continuation_command>
        operator()(ldb::field_found<ldb::store::pointer_type> found) const noexcept {
            std::ofstream("a.log", std::ios::app) << "AAAAA:" << found.value << "\n";
            auto commit = await(broadcast_delete(bcast, *found.value));
            std::ofstream("a.log", std::ios::app) << "BBBBB:" << found.value << "\n";
            if (!commit) return std::make_pair(false, continuation_command{});
            std::ofstream("a.log", std::ios::app) << "CCCCC:" << found.value << "\n";
            auto index_pointers = std::views::transform(indices, [](const index_type& idx) {
                return const_cast<index_type*>(std::addressof(idx));
            });

            return std::make_pair(false,
                                  continuation_command(TCommand{ldb::over_index<index_type>,
                                                                found.value,
                                                                index_pointers}));
        }

        ldb::store::broadcast_type& bcast;
        mutable std::span<typename TCommand::index_type> indices;
    };
}

std::optional<ldb::store::my_remove_command>
ldb::store::prepare_remove(const query_type& query) {
    broadcast_type yes = yes_mock_broadcast<void, void, MPI_Comm>{};
    std::unique_lock<std::shared_mutex> lck(_header_mtx);
    for (std::size_t i = 0; i < _header_indices.size(); ++i) {
        auto [cont, command] = remove_one(i, query, yes);
        if (cont) continue;
        return command;
    }
    return {}; // todo: remove_directly(query, yes);
}

std::optional<ldb::lv::linda_tuple>
ldb::store::remove_impl(const query_type& query, ldb::store::broadcast_type& bcast) {
    std::unique_lock<std::shared_mutex> lck(_header_mtx);
    for (std::size_t i = 0; i < _header_indices.size(); ++i) {
        auto [cont, command] = remove_one(i, query, bcast);
        if (cont) continue;
        if (!command) return {};
        return command->commit();
    }
    return remove_directly(query, bcast);
}

std::pair<bool, std::optional<ldb::store::my_remove_command>>
ldb::store::remove_one(std::size_t i, const query_type& query, ldb::store::broadcast_type& bcast) {
    const auto result = query.search_on_index(i, _header_indices[i]);
    return std::visit(query_result_visitor<my_remove_command>{bcast, _header_indices},
                      result);
}

std::optional<ldb::lv::linda_tuple>
ldb::store::remove_directly(const query_type& query, ldb::store::broadcast_type& bcast) {
    return {}; // todo
}

std::optional<ldb::lv::linda_tuple>
ldb::store::read(const query_type& query) const {
    std::unique_lock<std::shared_mutex> lck(_header_mtx);
    for (std::size_t i = 0; i < _header_indices.size(); ++i) {
        auto [cont, command] = read_one(i, query);
        if (cont) continue;
        if (!command) return {};
        return command->commit();
    }
    return read_directly(query);
}

std::pair<bool, std::optional<ldb::store::my_read_command>>
ldb::store::read_one(std::size_t i, const query_type& query) const {
    broadcast_type yes = yes_mock_broadcast<void, void, MPI_Comm>{};
    const auto result = query.search_on_index(i, _header_indices[i]);
    return std::visit(query_result_visitor<my_read_command>{yes, _header_indices},
                      result);
}

std::optional<ldb::lv::linda_tuple>
ldb::store::read_directly(const query_type& query) const {
    return _data.locked_find(query);
}
