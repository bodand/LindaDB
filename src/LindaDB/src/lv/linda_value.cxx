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
 * Originally created: 2024-10-10.
 *
 * src/LindaDB/src/lv/linda_value --
 *   
 */

#include <ldb/lv/linda_value.hxx>
#include <ldb/query/meta.hxx>

#include <pstl/glue_execution_defs.h>

#include "ldb/common.hxx"
#include "ldb/lv/linda_tuple.hxx"

namespace {
    struct pg_str_size_visitor {
        constexpr const static auto type_prefix_sz = 2;
        bool escape;
        bool full_type;

        template<std::integral I>
        std::size_t
        operator()(I i) const noexcept {
            if constexpr (std::is_unsigned_v<I>) {
                return type_prefix_sz + static_cast<std::size_t>(std::floor(std::log10(i))) + 1;
            }
            else {
                return type_prefix_sz + static_cast<std::size_t>(std::floor(std::log10(std::abs(i)))) + 1 + (i < 0);
            }
        }

        std::size_t
        operator()(const std::string& s) const noexcept {
            const auto escapes = escape ? std::count(s.begin(), s.end(), ',')
                                        : 0;
            return type_prefix_sz + s.size() + static_cast<std::size_t>(escapes);
        }

        template<std::floating_point F>
        std::size_t
        operator()(F f) const noexcept {
            return type_prefix_sz + static_cast<std::size_t>(std::snprintf(nullptr, 0, "%g", static_cast<double>(f)));
        }

        std::size_t
        operator()(ldb::lv::fn_call_holder) const noexcept {
            assert_that(false, "call holder cannot be serialized into pg");
            LDB_UNREACHABLE;
        }

        std::size_t
        operator()(ldb::lv::fn_call_tag) const noexcept {
            return type_prefix_sz;
        }

        std::size_t
        operator()(const ldb::lv::ref_type& ref) const noexcept {
            const auto real_len = static_cast<std::size_t>(std::floor(std::log10(ref.type_idx()))) + 1;
            if (full_type) return type_prefix_sz + real_len;
            return real_len;
        }
    };

    struct pg_serialize_into {
        char* begin;
        char* end;
        bool escape;

        template<std::integral I>
        void
        operator()(I i) const noexcept {
            std::to_chars(begin, end, i);
        }

        void
        operator()(const std::string& s) noexcept {
            if (!escape) {
                std::copy(s.begin(), s.end(), begin);
                return;
            }

            // todo simd?
            for (const char c : s) {
                assert_that(begin != end);
                if (c == ',') {
                    *begin = '\\';
                    ++begin;
                }
                *begin = c;
                ++begin;
            }
        }

        template<std::floating_point F>
        void
        operator()(F f) const noexcept {
            std::snprintf(begin, end - begin, "%g", static_cast<double>(f));
        }

        void
        operator()(ldb::lv::fn_call_holder) const noexcept {
            assert_that(false, "call holder cannot be serialized into pg");
            LDB_UNREACHABLE;
        }

        void
        operator()(ldb::lv::fn_call_tag) const noexcept {
            // nop
        }

        void
        operator()(const ldb::lv::ref_type& ref) const noexcept {
            std::to_chars(begin, end, ref.type_idx());
        }
    };

    char*
    pg_serialize_head(const ldb::lv::linda_value& lv,
                      char* begin,
                      const char* const end) {
        std::to_chars(begin, begin + 1, lv.index(), 16);
        ++begin;
        assert_that(begin != end);

        *begin = '@';
        ++begin;
        return begin;
    }

    void
    pg_serialize_body(const ldb::lv::linda_value& lv,
                      char* begin,
                      char* end,
                      const bool escape) {
        std::visit(pg_serialize_into(begin, end, escape), lv);
    }
}

std::size_t
ldb::lv::io::pg_str_size(const linda_value& lv, const bool escape) {
    return std::visit(pg_str_size_visitor{.escape = escape, .full_type = true}, lv);
}

void
ldb::lv::io::pg_serialize_to(const linda_value& lv, char* begin, char* end, const bool escape) {
    begin = pg_serialize_head(lv, begin, end);
    pg_serialize_body(lv, begin, end, escape);
}

std::string
ldb::lv::io::pg_serialize(const linda_value& lv, const bool escape) {
    std::string ret(pg_str_size(lv, escape), ' ');
    pg_serialize_to(lv, ret.data(), ret.data() + ret.size(), escape);
    return ret;
}

std::size_t
ldb::lv::io::pg_query_str_size(const linda_value& lv, const bool escape) {
    return std::visit(pg_str_size_visitor{.escape = escape, .full_type = false}, lv);
}

void
ldb::lv::io::pg_query_serialize_to(const linda_value& lv, char* begin, char* end, bool escape) {
    if (lv.index() != ldb::meta::index_of_type<ref_type, linda_value>)
        begin = pg_serialize_head(lv, begin, end);
    pg_serialize_body(lv, begin, end, escape);
}

std::string
ldb::lv::io::pg_query_serialize(const linda_value& lv, const bool escape) {
    std::string ret(pg_query_str_size(lv, escape), ' ');
    pg_query_serialize_to(lv, ret.data(), ret.data() + ret.size(), escape);
    return ret;
}
