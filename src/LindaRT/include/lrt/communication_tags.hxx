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

#include <ldb/common.hxx>

namespace lrt {
    enum class communication_tag : int {
        Terminate = 0xDB'00'01,
        Insert = 0xDB'00'02,
        Delete = 0xDB'00'03,
        Eval = 0xDB'00'04,
        Search = 0xDB'00'05,
        TrySearch = 0xDB'00'06,
        TryDelete = 0xDB'00'07,
    };

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
     *                      1         2         3
     *    bits:  |01234567890123456789012345678901|
     *   value:  |XDMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM|
     *  legend:   X : 1 = 0, a zero bit to ensure the tags are not negative,
     *                       since those are reserved for internal MPI messages
     *            D : 1 = 1, the special bit of ack messages, otherwise there
     *                       could be collisions with other communication_tag
     *                       tag values
     *            M : 31, the message differentiator, a monotonically
     *                    increasing value, per rank, that wraps around
     *                    when it reached its maximum value.
     *
     *          Since each rank gets sent back their own message tag, each
     *          tag only needs to be unique within a given rank, and not
     *          globally. As the message differentiator monot
     */
    constexpr const static unsigned ack_mask = 0x40000000;

    inline int
    make_ack_tag() {
        // this is probably guaranteed, but let's make sure, as I haven't
        // read the standard recently nor do I want to
        static_assert(sizeof(signed) == sizeof(unsigned),
                      "MPI tag packing requires signed and unsigned to be the same size");

        // counter needs to have its top two bits clear at all times
        // this allows to have the top-most bit cleared for MPI, and the next set
        // for differentiating acks from normal messages
        constexpr const static unsigned counter_mask = 0xC0000000;

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
