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
 * src/LindaDB/public/ldb/store_command/read_store_command --
 *   
 */
#ifndef LINDADB_READ_STORE_COMMAND_HXX
#define LINDADB_READ_STORE_COMMAND_HXX

#include <type_traits>

#include <ldb/lv/linda_tuple.hxx>

namespace ldb {
    template<class Index, class Pointer>
    struct read_store_command {
        using index_type = std::add_const_t<Index>;

        template<std::ranges::forward_range Rng>
        read_store_command(helper::over_index_impl<Index> /*type dispatch*/,
                           Pointer tuple,
                           Rng&& indices)
            requires(std::same_as<std::ranges::range_value_t<Rng>, Index*>)
             : _tuple_it(tuple) {
            std::ignore = indices;
        }

        lv::linda_tuple
        commit() {
            return *_tuple_it;
        }

    private:
        Pointer _tuple_it;
    };
}

#endif
