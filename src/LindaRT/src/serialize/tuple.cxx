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
 * Originally created: 2023-11-06.
 *
 * src/LindaRT/src/serialize/tuple --
 *   Implementations of the tuple de-/serialization protocol of LindaRT.
 */

#include <algorithm>
#include <bit>
#include <execution>
#include <numeric>
#include <ranges>

#include <ldb/lv/linda_value.hxx>
#include <ldb/common.hxx>
#include <lrt/serialize/tuple.hxx>

namespace {
    template<class T>
    struct fail {
        constexpr static const auto value = false;
    };

#ifdef __cpp_lib_byteswap
    using std::byteswap;
#else
    template<std::integral T>
    [[maybe_unused]] constexpr T
    byteswap(T value) noexcept {
        static_assert(std::has_unique_object_representations_v<T>,
                      "T may not have padding bits");
        auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
        std::ranges::reverse(value_representation);
        return std::bit_cast<T>(value_representation);
    }
#endif

#ifdef LINDA_RT_BIG_ENDIAN
    constexpr const auto comm_endian = std::endian::big;
#else
    constexpr const auto comm_endian = std::endian::little;
#endif

    template<std::integral T>
    constexpr T
    swap_unless_comm_endian(T val) noexcept {
        if constexpr (std::endian::big == std::endian::native) { // on big endian system
            if constexpr (comm_endian == std::endian::big) {     // comm order is big
                return val;
            }
            else { // comm order is little
                return byteswap(val);
            }
        }
        else if constexpr (std::endian::little == std::endian::native) { // on little endian system
            if constexpr (comm_endian == std::endian::big) {             // comm order is big
                return byteswap(val);
            }
            else { // comm order is little
                return val;
            }
        }
        else {
            static_assert(fail<T>::value, "mixed endian machines are not supported");
        }
    }

    struct value_size_calculator {
        std::size_t buf = 0;

        void
        operator()(std::int16_t /*ignore*/) { buf = sizeof(std::int16_t); }
        void
        operator()(std::uint16_t /*ignore*/) { buf = sizeof(std::uint16_t); }
        void
        operator()(std::int32_t /*ignore*/) { buf = sizeof(std::int32_t); }
        void
        operator()(std::uint32_t /*ignore*/) { buf = sizeof(std::uint32_t); }
        void
        operator()(std::int64_t /*ignore*/) { buf = sizeof(std::int64_t); }
        void
        operator()(std::uint64_t /*ignore*/) { buf = sizeof(std::uint64_t); }
        void
        operator()(const std::string& str) { buf = sizeof(std::string::size_type) + str.size(); }
        void
        operator()(float /*ignore*/) { buf = sizeof(float); }
        void
        operator()(double /*ignore*/) { buf = sizeof(double); }
    };

    enum class typemap : std::uint8_t {
        LRT_INT16 = 0,
        LRT_INT32 = 1,
        LRT_INT64 = 2,
        LRT_UINT16 = 3,
        LRT_UINT32 = 4,
        LRT_UINT64 = 5,
        LRT_STRING = 6,
        LRT_FLOAT = 7,
        LRT_DOUBLE = 8,
    };

    template<class>
    struct to_typemap;
    template<>
    struct to_typemap<std::int16_t> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_INT16);
    };
    template<>
    struct to_typemap<std::int32_t> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_INT32);
    };
    template<>
    struct to_typemap<std::int64_t> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_INT64);
    };
    template<>
    struct to_typemap<std::uint16_t> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_UINT16);
    };
    template<>
    struct to_typemap<std::uint32_t> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_UINT32);
    };
    template<>
    struct to_typemap<std::uint64_t> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_UINT64);
    };
    template<>
    struct to_typemap<std::string> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_STRING);
    };
    template<>
    struct to_typemap<float> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_FLOAT);
    };
    template<>
    struct to_typemap<double> {
        constexpr const static auto value = static_cast<std::byte>(typemap::LRT_DOUBLE);
    };

    struct value_serializator {
        static_assert(std::numeric_limits<float>::is_iec559,
                      "LindaRT requires IEEE754 floats");
        static_assert(std::numeric_limits<double>::is_iec559,
                      "LindaRT requires IEEE754 doubles");

        explicit
        value_serializator(std::byte* buf) : buf(buf) { }
        std::byte* buf;

        template<std::integral T>
        std::size_t
        operator()(T val) const {
            buf[0] = to_typemap<T>::value;
            write_int_raw(val, buf + 1);
            return 1 + sizeof(T);
        }

        std::size_t
        operator()(const std::string& str) const {
            buf[0] = to_typemap<std::string>::value;
            write_int_raw(str.size(), buf + 1);
            std::transform(std::execution::par_unseq,
                           str.data(),
                           str.data() + str.size(),
                           buf + 1 + sizeof(std::string::size_type),
                           [](const char c) {
                               return static_cast<std::byte>(c);
                           });
            return 1 + sizeof(std::string::size_type) + str.size();
        }

        template<std::floating_point T>
        std::size_t
        operator()(T val) const {
            auto value_representation =
                   std::bit_cast<std::array<std::byte, sizeof(std::remove_cvref_t<T>)>>(val);
            buf[0] = to_typemap<T>::value;
            std::copy(std::execution::par_unseq,
                      value_representation.begin(),
                      value_representation.end(),
                      buf + 1);
            return 1 + sizeof(T);
        }

        template<std::integral T>
        static std::size_t
        write_int_raw(T val, std::byte* buf) {
            auto value_representation =
                   std::bit_cast<std::array<std::byte, sizeof(std::remove_cvref_t<T>)>>(swap_unless_comm_endian(val));
            std::copy(std::execution::par_unseq,
                      value_representation.begin(),
                      value_representation.end(),
                      buf);
            return sizeof(T);
        }
    };

    std::size_t
    calculate_value_serial_size(const ldb::lv::linda_value& val) {
        value_size_calculator calc;
        std::visit(calc, val);
        return 1 + calc.buf; // 1 byte header
    }

    std::size_t
    calculate_tuple_serial_size(const ldb::lv::linda_tuple& tuple) {
        const auto size = sizeof(std::size_t); // tuple len
        std::vector<std::size_t> buf(tuple.size(), std::size_t{});
        std::iota(buf.begin(), buf.end(), std::size_t{});
        std::transform(std::execution::par_unseq, buf.begin(), buf.end(), buf.begin(), [&tuple](std::size_t i) {
            return calculate_value_serial_size(tuple[i]);
        });
        return size + std::reduce(std::execution::par_unseq, buf.begin(), buf.end());
    }

    std::size_t
    value_serialize(std::byte* buf,
                    const ldb::lv::linda_value& val) {
        const value_serializator ser{buf};
        return std::visit(ser, val);
    }

    std::pair<std::unique_ptr<std::byte[]>, std::size_t>
    tuple_serialize(const ldb::lv::linda_tuple& val) {
        const auto serial_size = calculate_tuple_serial_size(val) + 1;
        auto buf = std::make_unique<std::byte[]>(serial_size);
        buf[0] = std::byte{1};
        std::size_t i = 1;
        i += value_serializator::write_int_raw(val.size(), buf.get() + i);
        for (std::size_t j = 0; j < val.size(); ++j) {
            i += value_serialize(buf.get() + i, val[j]);
        }
        return std::make_pair(std::move(buf), serial_size);
    }

    template<std::constructible_from T>
    T
    deserialize_numeric(std::byte*& buf, std::size_t& len) {
        assert(len >= sizeof(T));
        T i{};
        auto value_representation =
               std::bit_cast<std::byte*>(&i);
        std::copy(std::execution::par_unseq,
                  buf,
                  buf + sizeof(T),
                  value_representation);
        len -= sizeof(T);
        buf += sizeof(T);
        if constexpr (std::integral<T>) {
            return swap_unless_comm_endian(i);
        }
        else {
            return i;
        }
    }

    ldb::lv::linda_value
    value_deserialize(std::byte*& buf, std::size_t& len) {
        --len;
        switch (static_cast<typemap>(*(buf++))) {
            using enum typemap;
        case LRT_INT16: return deserialize_numeric<std::int16_t>(buf, len);
        case LRT_INT32: return deserialize_numeric<std::int32_t>(buf, len);
        case LRT_INT64: return deserialize_numeric<std::int64_t>(buf, len);
        case LRT_UINT16: return deserialize_numeric<std::uint16_t>(buf, len);
        case LRT_UINT32: return deserialize_numeric<std::uint32_t>(buf, len);
        case LRT_UINT64: return deserialize_numeric<std::uint64_t>(buf, len);
        case LRT_FLOAT: return deserialize_numeric<float>(buf, len);
        case LRT_DOUBLE: return deserialize_numeric<double>(buf, len);
        case LRT_STRING: {
            const auto str_sz = deserialize_numeric<std::string::size_type>(buf, len);
            assert(len >= str_sz);
            std::string str(str_sz, '.');
            std::transform(std::execution::par_unseq,
                           buf,
                           buf + str_sz,
                           str.data(),
                           [](std::byte b) {
                               return static_cast<char>(b);
                           });
            len -= str_sz;
            buf += str_sz;
            return str;
        }
        }
        LDB_UNREACHABLE;
    }

    ldb::lv::linda_tuple
    tuple_deserialize(std::byte* buf, std::size_t len) {
        using namespace ldb::lv;
        std::vector<ldb::lv::linda_value> vals;
        const auto tuple_sz = deserialize_numeric<std::size_t>(buf, len);
        vals.reserve(tuple_sz);
        for (std::size_t i = 0; i < tuple_sz; ++i) {
            auto val = value_deserialize(buf, len);
            vals.emplace_back(val);
        }
        return ldb::lv::linda_tuple(vals);
    }
}

std::pair<std::unique_ptr<std::byte[]>, std::size_t>
lrt::serialize(const ldb::lv::linda_tuple& tuple) {
    return tuple_serialize(tuple);
}

ldb::lv::linda_tuple
lrt::deserialize(std::span<std::byte> buf) {
    return tuple_deserialize(buf.data() + 1, buf.size() - 1);
}
