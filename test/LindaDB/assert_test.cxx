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
 * Originally created: 2024-02-17.
 *
 * test/LindaDB/assert_test --
 *   
 */

#include <iostream>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <ldb/common.hxx>

namespace {
    template<class Strm>
    struct stream_resetter {
        void
        operator()(std::streambuf* buf) const {
            Strm::value->rdbuf(buf);
        }
    };

    template<std::istream* IStrm>
    struct istream_v {
        static inline auto value = IStrm;
    };

    template<std::ostream* OStrm>
    struct ostream_v {
        static inline auto value = OStrm;
    };

    template<class Strm>
    using reset_stream = std::unique_ptr<std::streambuf, stream_resetter<Strm>>;

    /**
     * \brief A function to capture output to an std::ostream.
     *
     * A function that can be used to capture all data output to the OStrm std::ostream object in string
     * format, as if it were to appear on wherever the stream was connected to.
     * It swaps the stream buffer under the stream allowing it to hijack all output. Note, that during
     * this function, the normal behavior of the stream may be compromised, for example, std::cout will
     * not write to STDOUT.
     * This is useful in case of only `std::ostream` objects that are not also `std::istream`-s, since
     * one cannot just read data from an `std::ostream`.
     *
     * The function is *not* thread safe.
     *
     * \tparam OStrm The std::ostream object to hijack. Passed as a pointer.
     * \tparam Fn The type of the function to execute while the stream is captured. Perfect forwarded.
     * \tparam Args The type of arguments passed to the executed function. Perfect forwarded.
     * \param fn The function to execute during the redirected stream state.
     * \param args The of arguments passed to the executed function.
     * \return The string data that would've been written to wherever the stream was connected.
     */
    template<std::ostream* OStrm, class Fn, class... Args>
    std::string
    capture_stream(Fn&& fn, Args&&... args) {
        std::ostringstream ss;
        { // REDIRECTION SCOPE
            auto buf_buf = reset_stream<ostream_v<OStrm>>(OStrm->rdbuf(ss.rdbuf()));
            std::forward<Fn>(fn)(std::forward<Args>(args)...);
        }
        return ss.str();
    }
}

TEST_CASE("successful assert_that prints nothing") {
    auto captured = capture_stream<&std::cerr>([] {
        assert_that(true);
    });
    CHECK(captured.empty());
}

TEST_CASE("failed assert_that prints things") {
    auto captured = capture_stream<&std::cerr>([] {
        assert_that(false);
    });
    CHECK_FALSE(captured.empty());
}

TEST_CASE("failed assert_that prints false expressions") {
    auto captured = capture_stream<&std::cerr>([] {
        assert_that(1 == 2);
    });
    CHECK(captured.contains("1 == 2"));
}

TEST_CASE("failed assert_that prints filename") {
    auto captured = capture_stream<&std::cerr>([] {
        assert_that(false);
    });
    CHECK(captured.contains(__FILE__));
}

TEST_CASE("failed assert_that with message prints message") {
    auto captured = capture_stream<&std::cerr>([] {
        assert_that(false, "xyz");
    });
    CHECK(captured.contains("xyz"));
}
