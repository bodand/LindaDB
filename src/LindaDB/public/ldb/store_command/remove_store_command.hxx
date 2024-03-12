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
 * Originally created: 2024-03-11.
 *
 * src/LindaDB/public/ldb/store_command/remove_store_command --
 *   
 */
#ifndef LINDADB_REMOVE_STORE_COMMAND_HXX
#define LINDADB_REMOVE_STORE_COMMAND_HXX

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <numeric>
#include <ranges>
#include <shared_mutex>
#include <tuple>
#include <vector>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/manual_fields_query.hxx>

namespace ldb {
    template<class Index, class Pointer>
    struct remove_store_command {
        using index_type = Index;

        template<std::ranges::forward_range Rng>
        remove_store_command(helper::over_index_impl<Index> /*type dispatch*/,
                             Pointer tuple,
                             Rng&& indices)
            requires(std::same_as<std::ranges::range_value_t<Rng>, Index*>)
             : _indices(std::ranges::begin(indices), std::ranges::end(indices)),
               tuple_it(tuple) { }

        lv::linda_tuple
        commit() {
            std::ranges::for_each(std::views::iota(std::size_t{}, _indices.size()),
                                  [this](std::size_t idx) {
                                      std::ignore = _indices[idx]->remove(index::tree::value_lookup((*tuple_it)[idx], tuple_it));
                                  });
            return *tuple_it;
        }

    private:
        std::vector<Index*> _indices;
        Pointer tuple_it;
    };
}

#endif
