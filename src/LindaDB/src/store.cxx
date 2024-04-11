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

#include <ldb/store.hxx>

namespace {
    template<class It>
    struct query_result_dispatcher {
        std::pair<bool, std::optional<It>>
        operator()(ldb::field_incomparable /*unused*/) const noexcept {
            return std::make_pair(true, std::nullopt);
        }

        std::pair<bool, std::optional<It>>
        operator()(ldb::field_not_found /*unused*/) const noexcept {
            return std::make_pair(false, std::nullopt);
        }

        std::pair<bool, std::optional<It>>
        operator()(ldb::field_found<ldb::store::pointer_type> found) const noexcept {
            return std::make_pair(false, std::optional(found.value));
        }
    };
}

std::optional<ldb::lv::linda_tuple>
ldb::store::remove_directly(const query_type& query) {
    LDBT_ZONE_A;
    return _data.locked_destructive_find(query);
}

std::optional<ldb::lv::linda_tuple>
ldb::store::read_directly(const query_type& query) const {
    LDBT_ZONE_A;
    return _data.locked_find(query);
}

std::optional<ldb::lv::linda_tuple>
ldb::store::perform_indexed_read(const query_type& query) const {
    LDBT_ZONE_A;
//    LDBT_UQ_LOCK(_header_mtx);
    for (std::size_t i = 0; i < _header_indices.size(); ++i) {
        LDBT_ZONE("indexed-read::index");
        const auto result = query.search_via_field(i, _header_indices[i]);
        const auto [cont, maybe_it] = std::visit(query_result_dispatcher<pointer_type>{}, result);
        if (cont) continue;
        if (!maybe_it) return {};
        return {**maybe_it};
    }
    return read_directly(query);
}

std::optional<ldb::lv::linda_tuple>
ldb::store::perform_indexed_remove(const ldb::store::query_type& query) {
    LDBT_ZONE_A;
//    LDBT_UQ_LOCK(_header_mtx);
    for (std::size_t i = 0; i < _header_indices.size(); ++i) {
        LDBT_ZONE("indexed-remove::index");
        const auto result = query.remove_via_field(i, _header_indices[i]);
        const auto [cont, maybe_it] = std::visit(query_result_dispatcher<pointer_type>{}, result);
        if (cont) continue;
        if (!maybe_it) return {};

        auto it = *maybe_it;
        std::ranges::for_each(std::views::iota(std::size_t{}, _header_indices.size()),
                              [it, this](std::size_t idx) {
                                  std::ignore = _header_indices[idx].remove(
                                         index::tree::value_lookup((*it)[idx], it));
                              });
        return *it;
    }
    return remove_directly(query);
}
