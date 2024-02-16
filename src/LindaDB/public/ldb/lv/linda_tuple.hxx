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

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <ostream>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <ldb/common.hxx>
#include <ldb/lv/linda_value.hxx>

namespace ldb::lv {
    namespace meta {
        template<class T>
        struct ensure_const_impl {
            using type = const T;
        };

        template<class T>
        struct ensure_const_impl<const T> {
            using type = const T;
        };

        template<class T>
        using ensure_const = ensure_const_impl<T>::type;

        template<class T, class>
        struct match_constness_of_impl {
            using type = T;
        };

        template<class T, class U>
        struct match_constness_of_impl<T, const U> {
            using type = ensure_const_impl<T>::type;
        };

        template<class T, class U>
        using match_constness_of = match_constness_of_impl<T, U>::type;
    }

    struct linda_tuple final {
        explicit linda_tuple()
             : _size(0) { }

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

        explicit linda_tuple(std::span<linda_value> vals)
             : _size(vals.size()) {
            if (vals.empty()) return;
            if (vals.size() <= 3) {
                std::ranges::copy(vals, _data_ref.begin());
            }
            else {
                using std::begin;
                using std::end;
                std::ranges::copy(begin(vals), std::next(begin(vals), 3), _data_ref.begin());
                if (vals.size() == 4) {
                    _tail = vals[3];
                }
                else {
                    _tail = std::vector<linda_value>(std::next(begin(vals), 3), end(vals));
                }
            }
        }

        [[nodiscard]] std::size_t
        size() const noexcept {
            return _size;
        }

        auto
        operator<=>(const linda_tuple& rhs) const noexcept {
            return _size <=> rhs._size;
        }

        [[nodiscard]] bool
        operator==(const linda_tuple& rhs) const noexcept = default;

        [[nodiscard]] linda_value&
        operator[](std::size_t idx) noexcept { return get_at(idx); }

        [[nodiscard]] const linda_value&
        operator[](std::size_t idx) const noexcept { return get_at(idx); }

    private:
        template<class ValueType>
        struct iterator_impl final {
            using difference_type = std::ptrdiff_t;
            using value_type = ValueType;
            using reference = std::add_lvalue_reference_t<value_type>;
            using const_reference = std::add_lvalue_reference_t<meta::ensure_const<value_type>>;
            using pointer = std::add_pointer_t<value_type>;
            using const_pointer = std::add_pointer_t<meta::ensure_const<ValueType>>;
            using iterator_category = std::random_access_iterator_tag;

            constexpr iterator_impl() = default;

            constexpr iterator_impl(const iterator_impl& cp) noexcept = default;
            constexpr iterator_impl&
            operator=(const iterator_impl& cp) noexcept = default;

            constexpr iterator_impl(iterator_impl&& cp) noexcept = default;
            constexpr iterator_impl&
            operator=(iterator_impl&& cp) noexcept = default;

            constexpr ~iterator_impl() = default;

            constexpr void
            swap(iterator_impl& other) noexcept {
                using std::swap;
                swap(_owner, other._owner);
                swap(_position, other._position);
            }

            [[nodiscard]] constexpr auto
            operator<=>(const iterator_impl& other) const noexcept {
                assert_that(arithmetic_meaningful_with(other));

                if (other._owner == nullptr && _owner == nullptr) return std::strong_ordering::equal;
                if (other._owner == nullptr) {
                    if (is_positional_end()) return std::strong_ordering::equal;
                    return std::strong_ordering::less;
                }
                if (_owner == nullptr) {
                    if (other.is_positional_end()) return std::strong_ordering::equal;
                    return std::strong_ordering::greater;
                }

                assert_that(_owner != nullptr);
                assert_that(other._owner != nullptr);

                return _position <=> other._position;
            }

            [[nodiscard]] constexpr bool
            operator==(const iterator_impl& other) const noexcept {
                return std::is_eq(*this <=> other);
            }

            [[nodiscard]] constexpr reference
            operator*() const noexcept(noexcept((*_owner)[_position])) {
                assert_that(!is_end());
                return (*_owner)[_position];
            }

            constexpr iterator_impl&
            operator++() noexcept {
                assert_that(!is_end());
                ++_position;
                return *this;
            }

            constexpr iterator_impl // NOLINT(*-dcl21-cpp)
            operator++(int) noexcept {
                assert_that(!is_end());
                const iterator_impl ret = *this;
                ++_position;
                return ret;
            }

            constexpr iterator_impl&
            operator+=(difference_type diff) noexcept {
                assert_that(valid_step(diff));
                if (diff > 0) _position += static_cast<std::size_t>(diff);
                if (diff < 0) _position -= static_cast<std::size_t>(-diff);
                return *this;
            }

            constexpr iterator_impl&
            operator--() noexcept {
                assert_that(!is_begin());
                --_position;
                return *this;
            }

            constexpr iterator_impl // NOLINT(*-dcl21-cpp)
            operator--(int) noexcept {
                assert_that(!is_begin());
                const iterator_impl ret = *this;
                --_position;
                return ret;
            }

            constexpr iterator_impl&
            operator-=(difference_type diff) noexcept {
                assert_that(valid_step(-diff));
                if (diff > 0) _position -= static_cast<std::size_t>(diff);
                if (diff < 0) _position += static_cast<std::size_t>(-diff);
                return *this;
            }

            [[nodiscard]] constexpr pointer
            operator->() const noexcept(noexcept((*_owner)[_position])) {
                assert_that(!is_end());
                return std::addressof((*_owner)[_position]);
            }

            [[nodiscard]] constexpr reference
            operator[](difference_type diff) const noexcept(noexcept((*_owner)[_position])) {
                assert_that(valid_step(diff));
                auto accessIdx = _position;
                if (diff > 0) accessIdx += static_cast<decltype(_position)>(diff);
                if (diff < 0) accessIdx -= static_cast<decltype(_position)>(-diff);
                return (*_owner)[accessIdx];
            }

        private:
            [[nodiscard]] friend constexpr auto
            operator+(difference_type diff, iterator_impl it) {
                auto ret = it;
                return ret += diff;
            }

            [[nodiscard]] friend constexpr auto
            operator+(iterator_impl it, difference_type diff) {
                auto ret = it;
                return ret += diff;
            }

            [[nodiscard]] friend constexpr auto
            operator-(iterator_impl it, difference_type diff) {
                auto ret = it;
                return ret -= diff;
            }

            [[nodiscard]] friend constexpr difference_type
            operator-(iterator_impl a, iterator_impl b) {
                assert_that(a.arithmetic_meaningful_with(b));
                if (a._owner == nullptr && b._owner == nullptr) return 0;
                if (a._owner == nullptr) return static_cast<difference_type>(b._owner->size() - b._position);
                if (b._owner == nullptr) return static_cast<difference_type>(a._position - a._owner->size());

                // prevent overflows with 1) value being too big for diff type
                //  or 2) size_t - size_t ending in negative (and getting wrapped)
                if (a._position > b._position) {
                    const auto diff = a._position - b._position;
                    // ...or fail screaming
                    assert_that(diff < static_cast<std::size_t>(std::numeric_limits<difference_type>::max()));
                    return static_cast<difference_type>(diff);
                }
                const auto diff = b._position - a._position;
                // ...or fail screaming
                assert(diff < static_cast<std::size_t>(std::numeric_limits<difference_type>::max()));

                return -static_cast<difference_type>(diff);
            }

            using tuple_type = meta::match_constness_of<linda_tuple, value_type>;
            friend linda_tuple;

            constexpr iterator_impl(tuple_type* owner, size_t position)
                 : _owner(owner), _position(position) { }

            [[nodiscard]] constexpr bool
            arithmetic_meaningful_with(iterator_impl other) const noexcept {
                if (_owner == nullptr) return true;
                if (other._owner == nullptr) return true;
                return _owner == other._owner;
            }

            [[nodiscard]] constexpr bool
            valid_step(difference_type by) const noexcept {
                if (!_owner) return false;
                if (by < 0) return _position >= static_cast<std::size_t>(-by);
                if (by > 0) return _owner->_size - _position >= static_cast<std::size_t>(by);
                return true;
            }

            [[nodiscard]] constexpr bool
            is_begin() const noexcept {
                return _position == 0;
            }

            [[nodiscard]] constexpr bool
            is_end() const noexcept {
                return _owner == nullptr
                       || is_positional_end();
            }

            [[nodiscard]] constexpr bool
            is_positional_end() const noexcept {
                assert(_owner);
                return _position == _owner->_size;
            }

            tuple_type* _owner{nullptr};
            std::size_t _position{static_cast<std::size_t>(-1)};
        };

        friend std::ostream&
        operator<<(std::ostream& os, const linda_tuple& tuple);

        [[nodiscard]] linda_value&
        get_at(std::size_t idx) noexcept;

        [[nodiscard]] const linda_value&
        get_at(std::size_t idx) const noexcept;

        std::size_t _size;
        std::array<linda_value, 3> _data_ref{};
        std::variant<std::monostate,
                     linda_value,
                     std::vector<linda_value>>
               _tail{};

    public:
        using iterator = iterator_impl<linda_value>;
        using const_iterator = iterator_impl<const linda_value>;

        [[nodiscard]] constexpr auto
        begin() { return iterator(this, 0); }
        [[nodiscard]] constexpr auto
        begin() const { return const_iterator(this, 0); }
        [[nodiscard]] constexpr auto
        cbegin() { return const_iterator(this, 0); }
        [[nodiscard]] constexpr auto
        cbegin() const { return const_iterator(this, 0); }

        [[nodiscard]] constexpr auto
        end() { return iterator(this, _size); }
        [[nodiscard]] constexpr auto
        end() const { return const_iterator(this, _size); }
        [[nodiscard]] constexpr auto
        cend() { return const_iterator(this, _size); }
        [[nodiscard]] constexpr auto
        cend() const { return const_iterator(this, _size); }
    };
}

namespace std {
    template<>
    struct hash<ldb::lv::linda_tuple> {
        constexpr std::size_t
        operator()(const ldb::lv::linda_tuple& tuple) const noexcept {
            std::size_t result = 0;
            for (const auto& val : tuple) {
                result ^= std::hash<ldb::lv::linda_value>{}(val);
            }
            return result;
        }
    };
}

#endif //LINDADB_LINDA_TUPLE_HXX
