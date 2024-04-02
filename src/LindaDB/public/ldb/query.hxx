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
 * Originally created: 2024-04-04.
 *
 * src/LindaDB/public/ldb/query --
 *   Collective header for the query API.
 */
#ifndef LINDADB_QUERY_HXX
#define LINDADB_QUERY_HXX

#include <ldb/query/concrete_tuple_query.hxx>
#include <ldb/query/manual_fields_query.hxx>
#include <ldb/query/tuple_query.hxx>
#include <ldb/query/tuple_query_if.hxx>
#include <ldb/query/type_stubbed_tuple_query.hxx>

namespace ldb {
    namespace helper {
        template<class T>
        struct over_index_impl { };
    }

    template<class T>
    constexpr const static auto over_index = helper::over_index_impl<T>{};

    template<class Index, class... Args>
    auto
    make_piecewise_query(helper::over_index_impl<Index> /* type-deduction */,
                         Args&&... args) {
        return manual_fields_query<Index, std::remove_cvref_t<Args>...>(
               std::forward<Args>(args)...);
    }

    template<class Index>
    auto
    make_concrete_query(helper::over_index_impl<Index> /* type-deduction */,
                        const lv::linda_tuple& tuple) {
        return concrete_tuple_query<Index>(tuple);
    }

    template<class Index>
    auto
    make_type_aware_query(helper::over_index_impl<Index> /* type-deduction */,
                          const lv::linda_tuple& tuple) {
        return type_stubbed_tuple_query<Index>(tuple);
    }
}

#endif
