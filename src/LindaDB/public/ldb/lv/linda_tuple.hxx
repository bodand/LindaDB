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
 * src/LindaDB/public/ldb/lv/linda_tuple --
 *   A statically sized element that stores heterogeneous linda_value types,
 *   as a tuple. The concrete types are unknown until runtime, but the linda_value
 *   abstraction makes them look homogeneous at compile time on our side.
 */

#ifndef LINDADB_LINDA_TUPLE_HXX
#define LINDADB_LINDA_TUPLE_HXX

#include <array>
#include <cassert>
#include <cstddef>
#include <vector>

#include <ldb/lv/linda_value.hxx>

namespace ldb::lv {
    struct linda_tuple final {
        explicit linda_tuple()
             : _size(0),
               _data_ref() { }

        explicit linda_tuple(linda_value lv1)
             : _size(1),
               _data_ref{std::move(lv1)} { }

        explicit linda_tuple(linda_value lv1, linda_value lv2)
             : _size(2),
               _data_ref{std::move(lv1), std::move(lv2)} { }

        explicit linda_tuple(linda_value lv1, linda_value lv2, linda_value lv3)
             : _size(3),
               _data_ref{std::move(lv1), std::move(lv2), std::move(lv3)} { }

        explicit linda_tuple(linda_value lv1, linda_value lv2, linda_value lv3, linda_value lv4)
             : _size(4),
               _data_ref{std::move(lv1), std::move(lv2), std::move(lv3)},
               _tail(std::move(lv4)) { }

        template<class... Args>
        explicit linda_tuple(linda_value lv1, linda_value lv2, linda_value lv3, linda_value lv4, Args&&... lvn)
            requires(sizeof...(Args) > 0)
             : _size(4 + sizeof...(Args)),
               _data_ref{std::move(lv1), std::move(lv2), std::move(lv3)},
               _tail(std::vector<linda_value>{lv4, linda_value(std::forward<Args>(lvn))...}) { }

        [[nodiscard]] std::size_t
        size() const noexcept {
            return _size;
        }

        auto
        operator<=>(const linda_tuple& rhs) const {
            return _size <=> rhs._size;
        }

        [[nodiscard]] bool
        operator==(const linda_tuple& rhs) const = default;

        [[nodiscard]] linda_value&
        operator[](std::size_t idx) { return get_at(idx); }

        [[nodiscard]] const linda_value&
        operator[](std::size_t idx) const { return get_at(idx); }

    private:
        [[nodiscard]] linda_value&
        get_at(std::size_t idx);

        [[nodiscard]] const linda_value&
        get_at(std::size_t idx) const;

        std::size_t _size;
        std::array<linda_value, 3> _data_ref;
        std::variant<std::monostate,
                     linda_value,
                     std::vector<linda_value>>
               _tail{};
    };


}

#endif //LINDADB_LINDA_TUPLE_HXX
