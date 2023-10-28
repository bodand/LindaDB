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
 * Originally created: 2023-10-14.
 *
 * src/LindaDB/public/ldb/index/tree/payload --
 *   Header file for defining the concept for what it means to be a payload of an
 *   index tree.
 */
#ifndef LINDADB_PAYLOAD_HXX
#define LINDADB_PAYLOAD_HXX


#include <concepts>
#include <optional>
#include <ostream>

namespace ldb::index::tree {
    // clang-format off
    template<class T>
    concept payload = requires(std::ostream& os, T payload) {
        typename T::key_type;
        typename T::value_type;
        typename T::size_type;
        typename T::bundle_type;

        { payload.capacity() } -> std::same_as<typename T::size_type>;
        { payload.size() } -> std::same_as<typename T::size_type>;
        { payload.full() } -> std::same_as<bool>;
        { payload.empty() } -> std::same_as<bool>;
        { os << payload } -> std::convertible_to<std::ostream&>;
    }
    && std::constructible_from<T>
    && std::constructible_from<T, typename T::key_type, typename T::value_type>
    && std::constructible_from<T, std::add_rvalue_reference_t<typename T::bundle_type>>
    && std::equality_comparable<typename T::key_type>
    && requires(T payload,
                typename T::key_type key,
                typename T::value_type value,
                typename T::bundle_type bundle) {
        { payload.try_get(key) } -> std::same_as<std::optional<typename T::value_type>>;
        { payload.try_set(key, value) } -> std::same_as<bool>;
        { payload.try_set(bundle) } -> std::same_as<bool>;
        { payload.force_set_lower(key, value) } -> std::same_as<std::optional<typename T::bundle_type>>;
        { payload.force_set_lower(std::move(bundle)) } -> std::same_as<std::optional<typename T::bundle_type>>;
        { payload.force_set_upper(key, value) } -> std::same_as<std::optional<typename T::bundle_type>>;
        { payload.force_set_upper(std::move(bundle)) } -> std::same_as<std::optional<typename T::bundle_type>>;
        { payload.remove(key) } -> std::same_as<std::optional<typename T::value_type>>;

        { payload <=> key } -> std::same_as<std::weak_ordering>;
        { payload == key } -> std::same_as<bool>;
        { payload != key } -> std::same_as<bool>;
        { key == payload } -> std::same_as<bool>;
        { key != payload } -> std::same_as<bool>;
    };
    // clang-format on

}

#endif
