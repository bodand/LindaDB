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
 * cmake/try_compile/have_fam --
 *   Source to check if flexible array members are available.
 *   Other than MSVC this should be supported, and is required for the Postgres
 *   backend.
 */

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <string_view>

struct fam_datum {
    unsigned char type: 8;
    unsigned long long data_sz: 64 - 8;
    char data[];

    void
    operator delete(void* p) {
        ::operator delete(p, static_cast<std::align_val_t>(alignof(fam_datum)));
    }

    static std::unique_ptr<fam_datum>
    create(std::string_view str) {
        return std::unique_ptr<fam_datum>(new (str) fam_datum(str));
    }
private:
    explicit
    fam_datum(std::string_view buf) : type(1), data_sz(buf.size()) {
        std::ranges::copy(buf, &data[0]);
    }

    // exmaple string-based allocator
    void*
    operator new(std::size_t count, std::string_view buf) {
        assert(count == sizeof(fam_datum));
        auto* storage = static_cast<fam_datum*>(::operator new(sizeof(fam_datum) + buf.size(),
                                                                static_cast<std::align_val_t>(alignof(fam_datum))));
        ::new (static_cast<void*>(storage)) fam_datum(buf);
        return storage;
    }
};

int
main() {
    std::string_view payload = "some data";
    auto fam = fam_datum::create(payload);

}
