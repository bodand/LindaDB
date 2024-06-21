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
 * Originally created: 2024-03-04.
 *
 * src/LindaRT/public/lrt/linda --
 *   
 */
#ifndef LINDADB_LINDA_HXX
#define LINDADB_LINDA_HXX

#include <lrt/runtime.hxx>
#include <lrt/runtime_storage.hxx>

template<class... Args>
inline void
out(Args&&... args) {
    LDBT_ZONE_A;
    lrt::this_runtime().out(ldb::lv::linda_tuple(std::forward<Args>(args)...));
}

template<class... Args>
auto
in(Args&&... args) {
    LDBT_ZONE_A;
    return lrt::this_runtime().in(std::forward<Args>(args)...);
}

template<class... Args>
bool
inp(Args&&... args) {
    LDBT_ZONE_A;
    return lrt::this_runtime().inp(std::forward<Args>(args)...);
}

template<class... Args>
auto
rd(Args&&... args) {
    LDBT_ZONE_A;
    return lrt::this_runtime().rd(std::forward<Args>(args)...);
}

template<class... Args>
bool
rdp(Args&&... args) {
    LDBT_ZONE_A;
    return lrt::this_runtime().rdp(std::forward<Args>(args)...);
}

#endif
