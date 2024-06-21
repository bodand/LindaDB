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
 * Originally created: 2024-04-04.
 *
 * src/LindaRT/include/lrt/work_pool/work/try_read_work --
 *   
 */
#ifndef LINDADB_TRY_READ_WORK_HXX
#define LINDADB_TRY_READ_WORK_HXX

#include <ostream>
#include <utility>
#include <vector>

namespace lrt {
    struct runtime;

    struct try_read_work {
        explicit try_read_work(std::vector<std::byte>&& payload,
                               runtime& runtime,
                               int sender,
                               int ack_with,
                               std::function<void(lrt::work<>)>)
             : _bytes(std::move(payload)),
               _runtime(&runtime),
               _sender(sender),
               _ack_with(ack_with) {
            LDBT_ZONE("try read work ctor");
        }

        void
        perform();

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const try_read_work& work);

        mutable std::vector<std::byte> _bytes;
        lrt::runtime* _runtime;
        int _sender;
        int _ack_with;
    };
}

#endif
