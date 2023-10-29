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
 * Originally created: 2023-10-29.
 *
 * src/LindaDB/public/ldb/data/chunked_list --
 *   A special container that provides stable iterators to their elements,
 *   like a list but also the iterators are comparable, like raw pointers in an array.
 *   Simply stores values in a chunk which is stored in a pointer in a vector.
 *   The chunks do not know about one another, but they know the chunked_list.
 *   Memory layout looks like the following, where letters are the stored values.
 *
 *  [H[A, B, A]
 *   \       [H[C, _, D]
 *    |     /
 *   [*, *, *]
 *       |
 *       [H[Y, B, A]
 *
 *  Upon insertion the data is put in the last chunk, or if it is full, in a newly
 *  allocated chunk. Since the iterators refer to the chunks themselves, if the vector
 *  is reallocated, only their pointers move around in memory, so insertion does not
 *  cause other fields to be invalidated.
 *
 *  Upon deletion, the value is removed from the chunk, but the chunk is not shuffled
 *  around, just the header marks the field as empty.
 *  This way, even in case of deletion the iterators to all other elements remain
 *  valid.
 *
 *  Iterators refer to their chunk, and contain an index to the value within the chunk.
 */
#ifndef LINDADB_CHUNKED_LIST_HXX
#define LINDADB_CHUNKED_LIST_HXX

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <vector>

namespace ldb::data {
    namespace meta {
        template<std::size_t Size>
        using count_size_t = std::conditional_t<
               Size <= sizeof(unsigned char) * CHAR_BIT,
               unsigned char,
               std::conditional_t<
                      Size <= sizeof(unsigned short) * CHAR_BIT,
                      unsigned short,
                      std::conditional_t<
                             Size <= sizeof(unsigned int) * CHAR_BIT,
                             unsigned int,
                             std::conditional_t<
                                    Size <= sizeof(unsigned long) * CHAR_BIT,
                                    unsigned long,
                                    std::enable_if_t<Size <= sizeof(unsigned long long) * CHAR_BIT,
                                                     unsigned long long>>>>>;
    }

    template<class T, std::size_t ChunkSize = 16ULL>
    struct chunked_list {
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;

    private:
        using ssize_type = std::make_signed_t<size_type>;
        using chunk_size_t = meta::count_size_t<ChunkSize>;
        static_assert(std::is_unsigned_v<chunk_size_t>);

        friend struct iterator_impl;
        friend struct data_chunk;

        struct data_chunk {
            [[nodiscard]] constexpr auto
            size() const noexcept {
                return std::popcount(_valids);
            }

            [[nodiscard]] constexpr auto
            capacity() const noexcept {
                return ChunkSize;
            }

            [[nodiscard]] constexpr auto
            full() const noexcept {
                return _valids == chunk_size_t{-1};
            }

            constexpr reference
            operator[](size_type idx) noexcept {
                assert(idx >= 0);
                assert(idx < ChunkSize);
                assert(valid_at_index(idx));
                return _data[idx];
            }

            constexpr const_reference
            operator[](size_type idx) const noexcept {
                assert(idx >= 0);
                assert(idx < ChunkSize);
                assert(valid_at_index(idx));
                return _data[idx];
            }

        private:
            data_chunk(chunked_list* owner, size_type chunk_index)
                 : _owner(owner),
                   _chunk_index(chunk_index) { }

            friend iterator_impl;

            [[nodiscard]] constexpr bool
            valid_at_index(std::size_t idx) const noexcept {
                return (_valids & (1U << idx)) != 0U;
            }

            [[nodiscard]] constexpr data_chunk*
            get_next_chunk(difference_type by = 1) {
                return _owner->next_chunk_after(_chunk_index, by / static_cast<difference_type>(ChunkSize));
            }

            chunked_list* _owner;
            std::size_t _chunk_index;
            chunk_size_t _valids{};
            std::array<T, ChunkSize> _data{};
        };

        struct iterator_impl {
            using difference_type = difference_type;
            using value_type = value_type;
            using reference = reference;
            using pointer = pointer;
            using iterator_category = std::random_access_iterator_tag;

            constexpr reference
            operator*() noexcept(noexcept((*_chunk)[_index])) {
                assert(!is_end());
                return (*_chunk)[_index];
            }

            constexpr reference
            operator*() const noexcept(noexcept((*_chunk)[_index])) {
                assert(!is_end());
                return (*_chunk)[_index];
            }

            constexpr iterator_impl&
            operator++() {
                if (_index < _chunk->size() - 1) {
                    ++_index;
                }
                else {
                    _chunk = _chunk->get_next_chunk();
                    _index = std::size_t{0};
                }
                return *this;
            }

            constexpr iterator_impl
            operator++(int) {
                iterator_impl ret = *this;
                ++(*this);
                return ret;
            }

            constexpr pointer
            operator->() noexcept {
                return &(*_chunk)[_index];
            }

            constexpr const_pointer
            operator->() const noexcept {
                return &(*_chunk)[_index];
            }

            [[nodiscard]] constexpr bool
            operator==(const iterator_impl& other) const noexcept = default;

            constexpr iterator_impl() = default;

            void
            swap(iterator_impl& other) noexcept {
                using std::swap;
                swap(_chunk, other._chunk);
                swap(_index, other._index);
            }

        private:
            constexpr iterator_impl(data_chunk* chunk, size_t index)
                 : _chunk(chunk),
                   _index(index) { }

            [[nodiscard]] constexpr bool
            is_end() const noexcept {
                return _chunk == nullptr;
            }

            data_chunk* _chunk{};
            size_type _index{};
        };

        data_chunk*
        next_chunk_after(size_type pos, difference_type diff) {
            if (_chunks.size() - pos < diff) return nullptr;
            if (static_cast<ssize_type>(pos) < -diff) return nullptr;
            const auto next_pos = static_cast<size_type>(static_cast<ssize_type>(pos) + diff);
            return _chunks[next_pos].get();
        }

        std::vector<std::unique_ptr<data_chunk>> _chunks;

    public:
        using iterator = iterator_impl;
        using const_iterator = const iterator_impl;
    };
}

#endif
