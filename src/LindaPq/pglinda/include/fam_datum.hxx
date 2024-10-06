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
 * Originally created: 2024-07-07.
 *
 * src/LindaPq/src/pqlinda/fam_datum --
 *   
 */
#ifndef FAM_DATUM_HXX
#define FAM_DATUM_HXX

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <ranges>
#include <ratio>
#include <string_view>
#include <tuple>

extern "C"
{
#include <postgres.h>
// after postgres.h
#include <libpq/pqformat.h>
#include <varatt.h>
}

class fam_datum {
    // Used internall by Postgres macros
    [[maybe_unused]] std::int32_t data_sz{};
    std::byte _data[];

    constexpr static struct call_tag_tag {
    } call_tag;

    constexpr static auto hdr_overhead = VARHDRSZ;
    /**
     * Specifies the amount of extra memory that needs to allocated.
     */
    constexpr static auto alloc_overhead = hdr_overhead + sizeof(unsigned char);

public:
    enum datum_type : unsigned char {
        SINT16 = 0,
        UINT16 = 1,
        SINT32 = 2,
        UINT32 = 3,
        STRING = 4,
        FNCALL = 5,
        SINT64 = 6,
        UINT64 = 7,
        FLOT32 = 8,
        FLOT64 = 9,
        FNCTAG = 10,
        TYPERF = 11
    };

    unsigned char
    type() const {
        return static_cast<unsigned char>(_data[0]);
    }

    void
    type(unsigned char type) {
        _data[0] = static_cast<std::byte>(type);
    }

    void
    operator delete(void* p) {
        pfree(p);
    }

    template<class T>
    static fam_datum*
    create(T&& param) {
        return new (std::forward<T>(param)) fam_datum(std::forward<T>(param));
    }

    static fam_datum*
    create_call_tag() {
        return new (call_tag) fam_datum(call_tag);
    }

    [[nodiscard]] std::size_t
    size() const { return VARSIZE(const_cast<fam_datum*>(this)) - alloc_overhead; }

    void
    size(std::size_t sz) {
        SET_VARSIZE(this, alloc_overhead + sz);
    }

    std::byte*
    data() {
        return &_data[1];
    }

    [[nodiscard]] const std::byte*
    data() const {
        return &_data[1];
    }

    bytea*
    to_bytes() {
        StringInfoData buf;
        pq_begintypsend(&buf);

        // there is no different function for signed/unsigned integer types
        // all take an unsigned value, so signed types are just fallthroughs

        pq_sendbyte(&buf, type());
        switch (static_cast<datum_type>(type())) {
        case SINT16:
            [[fallthrough]];
        case UINT16:
            pq_sendint16(&buf, *reinterpret_cast<std::uint16_t*>(data()));
            break;
        case SINT32:
            [[fallthrough]];
        case UINT32:
            pq_sendint32(&buf, *reinterpret_cast<std::uint32_t*>(data()));
            break;
        case STRING:
            pq_sendstring(&buf, reinterpret_cast<const char*>(data()));
            break;
        case FNCALL:
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("invalid data for type %s: %s not implemented",
                            "linda_value",
                            "fncall")));
        case SINT64:
            [[fallthrough]];
        case UINT64:
            pq_sendint64(&buf, *reinterpret_cast<std::uint64_t*>(data()));
            break;
        case FLOT32:
            pq_sendfloat4(&buf, *reinterpret_cast<float*>(data()));
            break;
        case FLOT64:
            pq_sendfloat8(&buf, *reinterpret_cast<double*>(data()));
            break;
        case FNCTAG: break; // nop
        case TYPERF:
            pq_sendbyte(&buf, *reinterpret_cast<std::uint8_t*>(data()));
            break;
        default:
            pg_unreachable();
        }

        return pq_endtypsend(&buf);
    }

    char*
    to_cstring() {
        switch (type()) {
        case SINT16:
            return psprintf("0@%" PRId16, *reinterpret_cast<std::int16_t*>(data()));
        case UINT16:
            return psprintf("1@%" PRIu16, *reinterpret_cast<std::uint16_t*>(data()));
        case SINT32:
            return psprintf("2@%" PRId32, *reinterpret_cast<std::int32_t*>(data()));
        case UINT32:
            return psprintf("3@%" PRIu32, *reinterpret_cast<std::uint32_t*>(data()));
        case STRING:
            return psprintf("4@%.*s", size(), reinterpret_cast<const char*>(data()));
        case SINT64:
            return psprintf("6@%" PRId64, *reinterpret_cast<std::uint64_t*>(data()));
        case UINT64:
            return psprintf("7@%" PRIu64, *reinterpret_cast<std::uint64_t*>(data()));
        case FLOT32:
            return psprintf("8@%g", *reinterpret_cast<float*>(data()));
        case FLOT64:
            return psprintf("9@%g", *reinterpret_cast<double*>(data()));
        case FNCTAG:
            return pstrdup("A@");
        case TYPERF:
            return psprintf("B@%" PRId8, size(), *reinterpret_cast<std::int8_t*>(data()));
        case FNCALL:
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("invalid data for type %s: %s not implemented",
                            "linda_value",
                            "fncall")));
        default:
            pg_unreachable();
        }
    }

    auto
    operator<=>(const fam_datum& other) const {
        const auto type_cmp = type() <=> other.type();
        if (std::is_neq(type_cmp)) return type_cmp;

        const auto size_cmp = size() <=> other.size();
        if (std::is_neq(size_cmp)) return size_cmp;

        // at this point, lvs have the same type and length
        const std::byte* lhs = data();
        const std::byte* rhs = other.data();
        for (std::size_t i = 0; i < size(); ++i) {
            const auto cmp = lhs[i] <=> rhs[i];
            if (std::is_neq(cmp)) return cmp;
        }

        return std::strong_ordering::equal;
    }
    auto
    operator==(const fam_datum& other) const {
        return std::is_eq(*this <=> other);
    }

private:
    explicit
    fam_datum(std::int8_t buf) {
        type(TYPERF);
        size(sizeof(buf));
        *data() = static_cast<std::byte>(buf);
    }

    template<std::integral Int>
    explicit fam_datum(Int number) {
        type(sizeof(Int)
             - sizeof(std::int16_t)
             + std::is_unsigned_v<Int>);
        size(sizeof(Int));
        *reinterpret_cast<Int*>(data()) = number;
    }

    explicit
    fam_datum(float f) {
        type(FLOT32);
        size(sizeof(f));
        const std::byte* buf = std::bit_cast<std::byte*>(&f);
        std::ranges::copy_n(buf, sizeof(float), data());
    }

    explicit
    fam_datum(double f) {
        type(FLOT64);
        size(sizeof(f));
        const std::byte* buf = std::bit_cast<std::byte*>(&f);
        std::ranges::copy_n(buf, sizeof(double), data());
    }


    explicit
    fam_datum(call_tag_tag /*tag*/) {
        type(FNCTAG);
        size(0);
    }

    explicit
    fam_datum(std::string_view buf) {
        type(STRING);
        size(buf.size());
        static_assert(sizeof(std::byte) == sizeof(const char));
        std::ranges::copy_n(reinterpret_cast<const std::byte*>(buf.data()),
                            static_cast<std::iter_difference_t<const std::byte*>>(buf.size()),
                            data());
    }

    void*
    operator new(std::size_t count, std::string_view buf) {
        std::ignore = count;
        assert(count == sizeof(fam_datum));
        int flags = MCXT_ALLOC_ZERO;

        // use huge allocations if allocation is more than a GB (SI)
        // this should fit even if 1 GiB is expected, since GB < GiB
        if (buf.size() + sizeof(fam_datum) > std::giga::num) flags |= MCXT_ALLOC_HUGE;

        auto* storage = static_cast<fam_datum*>(palloc_aligned(alloc_overhead + buf.size(),
                                                               alignof(fam_datum),
                                                               flags));
        return storage;
    }

    template<class T>
        requires std::integral<T> || std::floating_point<T>
    void*
    operator new(std::size_t count, T /*unused here*/) {
        std::ignore = count;
        assert(count == sizeof(fam_datum));
        auto* storage = static_cast<fam_datum*>(palloc_aligned(alloc_overhead + sizeof(T),
                                                               alignof(fam_datum),
                                                               MCXT_ALLOC_ZERO));
        return storage;
    }

    void*
    operator new(std::size_t count, call_tag_tag /* tag */) {
        std::ignore = count;
        assert(count == sizeof(fam_datum));
        auto* storage = static_cast<fam_datum*>(palloc_aligned(alloc_overhead,
                                                               alignof(fam_datum),
                                                               MCXT_ALLOC_ZERO));
        return storage;
    }
};

#endif
