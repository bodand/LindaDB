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

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <shared_mutex>
#include <syncstream>
#include <type_traits>
#include <vector>

#include <ldb/common.hxx>

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

        template<std::integral T>
        auto
        sgn(T val) noexcept(noexcept((T(0) < val) - (val < T(0)))) {
            return static_cast<T>(static_cast<T>(0) < val) - static_cast<T>(val < static_cast<T>(0));
        }

        template<class T>
        auto
        div_and_round_to_x(T x, T y) {
            return (static_cast<T>(1) + ((std::abs(x) - static_cast<T>(1)) / y)) * sgn(x);
        }
    }

    // todo allocator where?
    template<class T, std::size_t ChunkSize = 16ULL>
    struct chunked_list {
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;

        constexpr
        chunked_list() = default;

        constexpr explicit(false) chunked_list(std::initializer_list<T> initializer_list)
             : _chunks(1 + ((initializer_list.size() - 1) / ChunkSize)) {
            std::ranges::generate(_chunks, [] { return std::make_unique<data_chunk>(); });
            for (std::size_t i = 0;
                 auto&& it : initializer_list) {
                auto chunk_idx = i / ChunkSize;
                _chunks[chunk_idx]->emplace_back(std::forward<T>(it));
                ++i;
            }
        }

        constexpr
        chunked_list(size_type count, const value_type& value)
             : _chunks(1 + ((count - 1) / ChunkSize)) {
            std::ranges::generate(_chunks, [] { return std::make_unique<data_chunk>(); });
            for (size_type i = 0; i < count; ++i) {
                auto chunk_idx = i / ChunkSize;
                _chunks[chunk_idx]->emplace_back(value);
            }
        }

        constexpr explicit
        chunked_list(size_type count)
             : _chunks(1 + ((count - 1) / ChunkSize)) {
            std::ranges::generate(_chunks, [] { return std::make_unique<data_chunk>(); });
            for (size_type i = 0; i < count; ++i) {
                auto chunk_idx = i / ChunkSize;
                _chunks[chunk_idx]->emplace_back();
            }
        }

        template<std::input_iterator It>
        constexpr chunked_list(It begin, It end)
             : _chunks(1, std::make_unique<data_chunk>()) {
            size_type i = 0;
            for (auto p = begin; p != end; ++p) {
                auto chunk_idx = i / ChunkSize;
                _chunks[chunk_idx]->emplace_back(*p);
                ++i;
            }
        }

        constexpr
        chunked_list(const chunked_list& cp)
             : _chunks(cp._chunks.size()) {
            std::ranges::transform(cp._chunks, _chunks.begin(), [](const auto& chunk_ptr) {
                return std::make_unique<data_chunk>(*chunk_ptr);
            });
        }
        constexpr
        chunked_list(chunked_list&& cp) noexcept = default;

        constexpr chunked_list&
        operator=(const chunked_list& cp) {
            if (this == &cp) return *this;
            _chunks.clear();
            _chunks.reserve(cp._chunks.size());
            std::ranges::transform(cp._chunks, std::back_inserter(_chunks), [](const auto& chunk_ptr) {
                return std::make_unique<data_chunk>(*chunk_ptr);
            });
            return *this;
        }
        constexpr chunked_list&
        operator=(chunked_list&& cp) noexcept = default;

        // clang-format off
        constexpr ~chunked_list() noexcept = default;
        // clang-format on

        [[nodiscard]] LDB_CONSTEXPR23 bool
        empty() const noexcept {
            std::shared_lock lck(_mtx);
            return _chunks.empty() || (_chunks.size() == 1 && _chunks[0]->empty());
        }

        [[nodiscard]] LDB_CONSTEXPR23 auto
        size() const noexcept {
            std::shared_lock lck(_mtx);
            return std::reduce(
                   _chunks.begin(),
                   _chunks.end(),
                   size_type{},
                   []<class T2, class U2>(const T2& chunk_or_sum_1, const U2& chunk_or_sum_2) {
                       if constexpr (std::same_as<T2, size_type>
                                     && std::same_as<U2, size_type>) {
                           return chunk_or_sum_1 + chunk_or_sum_2;
                       }
                       else if constexpr (std::same_as<T2, std::unique_ptr<data_chunk>>
                                          && std::same_as<U2, size_type>) {
                           return chunk_or_sum_1->size() + chunk_or_sum_2;
                       }
                       else if constexpr (std::same_as<T2, size_type>
                                          && std::same_as<U2, std::unique_ptr<data_chunk>>) {
                           return chunk_or_sum_1 + chunk_or_sum_2->size();
                       }
                       else {
                           return chunk_or_sum_1->size() + chunk_or_sum_2->size();
                       }
                   });
        }

        [[nodiscard]] LDB_CONSTEXPR23 bool
        capacity() const noexcept {
            std::shared_lock lck(_mtx);
            return _chunks.size() * ChunkSize;
        }

        LDB_CONSTEXPR23 void
        clear() {
            std::scoped_lock lck(_mtx);
            _chunks.clear();
        }

    private:
        using ssize_type = std::make_signed_t<size_type>;
        using chunk_size_t = meta::count_size_t<ChunkSize>;
        static_assert(std::is_unsigned_v<chunk_size_t>);

        struct iterator_impl;
        struct data_chunk;

        struct data_chunk {
            [[nodiscard]] constexpr unsigned
            size() const noexcept {
                return static_cast<unsigned>(std::popcount(_valids));
            }

            [[nodiscard]] constexpr auto
            capacity() const noexcept {
                return ChunkSize;
            }

            [[nodiscard]] constexpr auto
            full() const noexcept {
                // todo non-2^n size chunks break
                return _valids == static_cast<chunk_size_t>(-1);
            }

            [[nodiscard]] constexpr auto
            empty() const noexcept {
                // todo non-2^n size chunks break?
                return _valids == chunk_size_t{0};
            }

            [[nodiscard]] constexpr reference
            operator[](size_type idx) noexcept {
                assert(idx < ChunkSize);
                assert(valid_at_index(idx));
                return *std::bit_cast<pointer>(&_data[idx * sizeof(T)]);
            }

            [[nodiscard]] constexpr const_reference
            operator[](size_type idx) const noexcept {
                assert(idx < ChunkSize);
                assert(valid_at_index(idx));
                return *std::bit_cast<const_pointer>(&_data[idx * sizeof(T)]);
            }

            template<class... Args>
            auto
            emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>) {
                auto next_idx = static_cast<unsigned>(std::countr_one(_valids));
                assert(next_idx != ChunkSize);
                assert(!valid_at_index(next_idx));
                _valids |= (1U << next_idx);
                std::construct_at(std::bit_cast<pointer>(&_data[next_idx * sizeof(T)]),
                                  std::forward<Args>(args)...);
                return next_idx;
            }

            auto
            push(const_reference obj) noexcept(std::is_nothrow_copy_constructible_v<value_type>) {
                auto next_idx = static_cast<unsigned>(std::countr_one(_valids));
                assert(next_idx != ChunkSize);
                assert(!valid_at_index(next_idx));
                _valids |= (1U << next_idx);
                std::construct_at(std::bit_cast<pointer>(&_data[next_idx * sizeof(T)]),
                                  obj);
                return next_idx;
            }

            void
            destroy_at_index(size_type idx) {
                assert(valid_at_index(idx));
                _valids &= ~(1U << idx);
                std::destroy_at(std::bit_cast<pointer>(&_data[idx * sizeof(T)]));
                assert(!valid_at_index(idx));
            }

            [[nodiscard]] constexpr bool
            valid_at_index(size_type idx) const noexcept {
                return (_valids & (1U << idx)) != 0U;
            }

            data_chunk(chunked_list* owner, size_type chunk_index)
                 : _owner(owner),
                   _chunk_index(chunk_index) { }

            [[nodiscard]] constexpr data_chunk*
            get_next_chunk(difference_type by = 1) const noexcept {
                return _owner->next_chunk_after(_chunk_index, by);
            }

            [[nodiscard]] constexpr auto
            operator<=>(const data_chunk& other) const noexcept {
                return _chunk_index <=> other._chunk_index;
            }

            [[nodiscard]] constexpr auto
            is_final() const noexcept {
                return _owner->_chunks.size() - 1 == _chunk_index;
            }

            [[nodiscard]] constexpr auto
            is_head() const noexcept {
                return _chunk_index == 0;
            }

            ~
            data_chunk() noexcept {
                if (empty()) return;
                for (std::size_t i = 0; i < ChunkSize; ++i) {
                    if (!valid_at_index(i)) continue;
                    std::destroy_at(std::bit_cast<pointer>(&_data[i * sizeof(T)]));
                }
            }

            void
            set_index(std::size_t idx) {
                _chunk_index = idx;
            }

        private:
            friend chunked_list;
            mutable std::shared_mutex _chunk_mtx;
            const chunked_list* const _owner;
            size_type _chunk_index;
            chunk_size_t _valids{};
            alignas(alignof(T)) std::array<std::byte, sizeof(T) * ChunkSize> _data{std::byte{}};
        };

        struct iterator_impl {
            using difference_type = chunked_list::difference_type;
            using value_type = chunked_list::value_type;
            using reference = chunked_list::reference;
            using pointer = chunked_list::pointer;
            using iterator_category = std::bidirectional_iterator_tag;

            constexpr reference
            operator*() const noexcept(noexcept((*_chunk)[_index])) {
                return (*_chunk)[_index];
            }

            constexpr iterator_impl&
            operator++() {
                inc(1);
                return *this;
            }

            constexpr iterator_impl // NOLINT(*-dcl21-cpp)
            operator++(int) {
                iterator_impl ret = *this;
                inc(1);
                return ret;
            }

            constexpr iterator_impl&
            operator--() {
                dec(1);
                return *this;
            }

            constexpr iterator_impl // NOLINT(*-dcl21-cpp)
            operator--(int) {
                iterator_impl ret = *this;
                dec(1);
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

            [[nodiscard]] constexpr auto
            operator<=>(const iterator_impl& other) const noexcept {
                if (!is_end() && other.is_end()) return std::strong_ordering::less;
                if (is_end() && !other.is_end()) return std::strong_ordering::greater;
                if (is_end() && other.is_end()) return std::strong_ordering::equal;
                if (_chunk == nullptr
                    && other._chunk == nullptr) return std::strong_ordering::equal;

                if (const auto chunk_idx_order = (*_chunk) <=> (*other._chunk);
                    !std::is_eq(chunk_idx_order)) return chunk_idx_order;
                return _index <=> other._index;
            }

            [[nodiscard]] constexpr auto
            operator==(const iterator_impl& other) const noexcept {
                return std::is_eq(*this <=> other);
            }
            [[nodiscard]] constexpr auto
            operator!=(const iterator_impl& other) const noexcept {
                return std::is_neq(*this <=> other);
            }

            constexpr
            iterator_impl() = default;

            void
            swap(iterator_impl& other) noexcept {
                using std::swap;
                swap(_chunk, other._chunk);
                swap(_index, other._index);
            }


        private:
            friend chunked_list;
            constexpr
            iterator_impl(data_chunk* chunk, size_t index)
                 : _chunk(chunk),
                   _index(index) { }

            void
            inc(size_type by = 1U) {
                while (by != 0) {
                    while (++_index < _chunk->capacity()) {
                        if (_chunk->valid_at_index(_index)) --by;
                        if (by == 0) return;
                    }
                    if (_chunk->is_final()) {
                        _index = ChunkSize - static_cast<unsigned>(std::countl_zero(_chunk->_valids));
                        return;
                    }
                    _chunk = _chunk->get_next_chunk(1);
                    _index = 0;
                    if (!_chunk) return;
                    if (_chunk->valid_at_index(_index)) --by;
                }
            }

            void
            dec(size_type by = 1U) {
                while (by != 0) {
                    while (--_index < static_cast<std::size_t>(-1)) {
                        if (_chunk->valid_at_index(_index)) --by;
                        if (by == 0) return;
                    }
                    if (_chunk->is_head()) {
                        _index = static_cast<size_type>(-1);
                        return;
                    }
                    _chunk = _chunk->get_next_chunk(-1);
                    _index = 0;
                    if (!_chunk) return;
                    _index = _chunk->size();
                    if (_chunk->valid_at_index(_index)) --by;
                }
            }

            [[nodiscard]] constexpr bool
            is_end() const noexcept {
                return _chunk && _chunk->is_final() && _index == _chunk->size();
            }

            data_chunk* _chunk{};
            size_type _index{};
        };

        data_chunk*
        next_chunk_after(size_type pos, difference_type diff) const noexcept {
            if (static_cast<difference_type>(_chunks.size() - pos) < diff) return _chunks.back().get();
            if (static_cast<ssize_type>(pos) < -diff) return _chunks.back().get();
            const auto next_pos = static_cast<size_type>(static_cast<ssize_type>(pos) + diff);
            return _chunks[next_pos].get();
        }

        mutable std::shared_mutex _mtx;
        std::vector<std::unique_ptr<data_chunk>> _chunks;

    public:
        using iterator = iterator_impl;

        [[nodiscard]] LDB_CONSTEXPR23 iterator
        begin() const noexcept {
            std::shared_lock lck(_mtx);
            return begin_unguarded();
        }

        [[nodiscard]] LDB_CONSTEXPR23 iterator
        end() const noexcept {
            std::shared_lock lck(_mtx);
            return end_unguarded();
        }

        iterator
        push_back(const T& obj) {
            std::scoped_lock lck(_mtx);
            auto& back = [this]() -> decltype(auto) {
                if (_chunks.empty() || _chunks.back()->full()) {
                    _chunks.emplace_back(std::make_unique<data_chunk>(this, _chunks.size()));
                }
                return _chunks.back();
            }();
            auto inserted_idx = back->push(obj);
            return iterator(back.get(), inserted_idx);
        }

        template<class... Args>
        iterator
        emplace_back(Args&&... args) {
            std::scoped_lock lck(_mtx);
            auto& back = [this]() -> decltype(auto) {
                if (_chunks.empty() || _chunks.back()->full()) {
                    _chunks.emplace_back(std::make_unique<data_chunk>(this, _chunks.size()));
                }
                return _chunks.back();
            }();
            auto inserted_idx = back->emplace(std::forward<Args>(args)...);
            return iterator(back.get(), inserted_idx);
        }

        LDB_CONSTEXPR23 void
        erase(iterator it) noexcept {
            std::scoped_lock lck(_mtx);
            erase_unguarded(it);
        }

        template<class Q>
        LDB_CONSTEXPR23 std::optional<T>
        locked_destructive_find(Q&& query)
            requires(std::copyable<T>)
        {
            std::scoped_lock lck(_mtx);
            auto last = end_unguarded();
            auto found = std::ranges::find_if(begin_unguarded(), last, [query = std::forward<Q>(query)](const auto& stored) {
                return stored == query;
            });
            if (found == last) return std::nullopt;

            auto ret = std::optional{*found};
            erase_unguarded(found);
            return ret;
        }

        template<class Q>
        LDB_CONSTEXPR23 std::optional<T>
        locked_find(Q&& query) const
            requires(std::copyable<T>)
        {
            std::scoped_lock lck(_mtx);
            auto last = end_unguarded();
            auto found = std::ranges::find_if(begin_unguarded(), last, [query = std::forward<Q>(query)](const auto& stored) {
                return stored == query;
            });
            if (found == last) return std::nullopt;
            return std::optional{*found};
        }

    private:
        iterator
        begin_unguarded() const {
            if (_chunks.size() == 0) return iterator();
            return iterator(_chunks[0].get(),
                            static_cast<unsigned>(std::countr_zero(_chunks[0]->_valids)));
        }

        iterator
        end_unguarded() const {
            if (_chunks.empty()) return iterator();
            auto&& back = _chunks.back();
            return iterator(back.get(),
                            ChunkSize - static_cast<unsigned>(std::countl_zero(back->_valids)));
        }

        LDB_CONSTEXPR23 void
        erase_unguarded(iterator it) {
            auto chunk = it._chunk;
            const auto it_idx = it._index;
            assert(chunk);
            assert(chunk->valid_at_index(it_idx));

            chunk->destroy_at_index(it_idx);
            if (chunk->empty()) {
                _chunks.erase(next(_chunks.begin(),
                                   static_cast<difference_type>(chunk->_chunk_index)));
                for (std::size_t i = 0; i < _chunks.size(); ++i) {
                    _chunks[i]->set_index(i);
                }
            }
        }
    };
}

#endif
