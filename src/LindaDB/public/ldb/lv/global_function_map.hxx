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
 * Originally created: 2024-02-28.
 *
 * src/LindaRT/public/lrt/global_function_map --
 *   
 */
#ifndef LINDADB_GLOBAL_FUNCTION_MAP_HXX
#define LINDADB_GLOBAL_FUNCTION_MAP_HXX

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/lv/linda_value.hxx>

namespace ldb::lv {
    struct string_hash {
        using is_transparent = std::true_type;
        [[nodiscard]] std::size_t
        operator()(const char* txt) const {
            LDBT_ZONE_A;
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] std::size_t
        operator()(std::string_view txt) const {
            LDBT_ZONE_A;
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] std::size_t
        operator()(const std::string& txt) const {
            LDBT_ZONE_A;
            return std::hash<std::string>{}(txt);
        }
    };

    using global_function_map_type = std::unordered_map<
           std::string,
           std::function<ldb::lv::linda_value(const ldb::lv::linda_tuple&)>,
           string_hash,
           std::equal_to<>>;
    [[maybe_unused]] extern std::unique_ptr<global_function_map_type> gLdb_Dynamic_Function_Map;
}

#endif
