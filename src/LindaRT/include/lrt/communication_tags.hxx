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
 * Originally created: 2024-03-02.
 *
 * src/LindaRT/include/lrt/communication_tags --
 *   Defines the set of tags for differentiating the messages sent.
 *   Used in low-level communication, before work objects are constructed.
 */
#ifndef LINDADB_COMMUNICATION_TAGS_HXX
#define LINDADB_COMMUNICATION_TAGS_HXX

#include <atomic>
#include <compare>
#include <cstdint>
#include <limits>

#include <ldb/common.hxx>
#include <ldb/profile.hxx>

namespace lrt {
    // clang-format off
    enum class communication_tag : int {
        Terminate = 0b00'00000000000000,
        Insert    = 0b00'00000000000001,
        Delete    = 0b00'00000000000010,
        Eval      = 0b00'00000000000011,
        Search    = 0b00'00000000000100,
        TrySearch = 0b00'00000000000101,
        TryDelete = 0b00'00000000000111,
    };
    // clang-format on

    /*
     * ACK TAG: Since multiple acks could arrive from the same host,
     *          and their ordering is not guaranteed to be in the correct
     *          order, (eval#1r1 sends remove, eval#2r1 sends try_remove,
     *          neither are in the db, thus we need to make sure eval#1n1
     *          does not catch the ack of eval#2n1, and since are both on
     *          rank 1 (*r1), the basic MPI sender, receiver int value does
     *          not help) so everyone waits a special ack value which they
     *          send to r0 to reply to their answer with. This is a special
     *          bitmap, stuffed into an MPI tag, ie. an int.
     *          However, the MPI specification only guarantees the value
     *          range of a 16-bit int. (Which is valid, considering that is
     *          allowed in C and C++ as well.)
     *          Therefore, even if a given platform has 32 bit ints, an
     *          MPI implementation is not guaranteed to provide support for
     *          using the upper 16 bits...
     *
     *           |          1     |
     *    bits:  |0123456789012345|
     *   value:  |XDMMMMMMMMMMMMMM|
     *  legend:   X : 1 = 0, a zero bit to ensure the tags are not negative,
     *                       since those are reserved for internal MPI messages
     *            D : 1 = 1, the special bit of ack messages, otherwise there
     *                       could be collisions with other communication_tag
     *                       tag values (normal messages have D = 0)
     *            M : 15, the message differentiator, a monotonically
     *                    increasing value, per rank, that wraps around
     *                    when it reached its maximum value.
     *
     *          Since each rank gets sent back their own message tag, each
     *          tag only needs to be unique within a given rank, and not
     *          globally. As the message differentiator monot
     */
    constexpr const static unsigned ack_mask = 0b01'00000000000000;

    inline int
    make_ack_tag() {
        LDBT_ZONE_A;
        // this is probably guaranteed, but let's make sure, as I haven't
        // read the standard recently nor do I want to
        static_assert(sizeof(signed) == sizeof(unsigned),
                      "MPI tag packing requires signed and unsigned to be the same size");

        // counter needs to have its top two bits clear at all times
        // this allows to have the top-most bit cleared for MPI, and the next set
        // for differentiating acks from normal messages (even with 16-bit ints)
        constexpr const static unsigned counter_mask = 0b11'00000000000000;

        static std::atomic<unsigned> _counter = 0;
        auto value = _counter.load(std::memory_order::acquire);
        for (;;) {
            const auto next_value = (value + 1U) & ~counter_mask;
            if (_counter.compare_exchange_strong(value,
                                                 next_value,
                                                 std::memory_order::release,
                                                 std::memory_order::acquire))
                break;
        }
        assert_that(static_cast<int>(value | ack_mask) > 0,
                    "MPI requirements for tags to be positive");
        assert_that(static_cast<int>(value | ack_mask) <= std::numeric_limits<std::int16_t>::max(),
                    "MPI only guarantees for tags to be allowed to be smaller than a 16 bit signed int's max value");
        return static_cast<int>(value | ack_mask);
    }

    [[nodiscard]] constexpr auto
    operator<=>(int int_tag, communication_tag tag) noexcept {
        return int_tag <=> static_cast<int>(tag);
    }
    [[nodiscard]] constexpr auto
    operator==(int int_tag, communication_tag tag) noexcept {
        return std::is_eq(int_tag <=> tag);
    }
}

#endif
