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
 * test/simple --
 *   
 */

#include <chrono>
#include <cstring>

#include <ldb/lv/linda_tuple.hxx>
#include <ldb/query/match_type.hxx>
#include <lrt/runtime.hxx>

using namespace std::literals;

#include <mpi.h>

namespace lrt {
    static lrt::runtime* gLrt_Runtime_ObjectRef;

    lrt::runtime&
    this_runtime() {
        assert_that(gLrt_Runtime_ObjectRef, "runtime has not yet been initialized");
        return *gLrt_Runtime_ObjectRef;
    }

    ldb::store&
    this_store() {
        assert_that(gLrt_Runtime_ObjectRef, "runtime has not yet been initialized");
        return gLrt_Runtime_ObjectRef->store();
    }
}

#define out(...) lrt::this_store().out(ldb::lv::linda_tuple(__VA_ARGS__))
#define in(...) lrt::this_store().in(__VA_ARGS__)
#define inp(...) lrt::this_store().inp(__VA_ARGS__)
#define rd(...) lrt::this_store().rd(__VA_ARGS__)
#define rdp(...) lrt::this_store().rdp(__VA_ARGS__)

std::size_t
string_size(const char* str) {
    int adage = 0;
    //    in("str_adage", ldb::ref(&adage));
    std::cout << "adage: " << adage << "\n";
    return std::strlen(str) + adage;
}

int
real_main() {
        std::cout << "starting job...\n";
        eval("str_size", (string_size) ("test"));
        std::cout << "waiting in main process...\n";

        out("str_size", 38);

        std::size_t size;
        in("str_size", ldb::ref(&size));
        std::cout << "test size: " << size << "\n";

    return 0;
}

int
main(int argc, char** argv) {
    lrt::runtime rt(&argc, &argv);
    lrt::gLrt_Runtime_ObjectRef = &rt;

    //    volatile int i = 1;
    //    if (rt.rank() == 0)
    //        while (i == 1) std::this_thread::sleep_for(std::chrono::seconds(1));

    int result = 0;
    if (rt.rank() == 0) result = real_main();

    rt.loop();
    return result;
}
