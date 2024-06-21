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
 * Originally created: 2024-05-05.
 *
 * demo/build-sys/exec --
 *   
 */

#include "exec.hxx"

#include <iostream>
#include <stdexcept>
#include <string>

#include <Windows.h>

#include <shlwapi.h>

namespace {
    std::string
    get_exec(const std::string& name) {
        auto ret = PathFindFileNameA(name.c_str());
        if (ret == name.c_str()) return name;
        return {ret};
    }
}

int
cross_exec(const std::string& exe, const std::string& args) {
    const auto full_exe = get_exec(exe);
    auto full_args = full_exe + " " + args;

    STARTUPINFOA startup{
           .cb = sizeof startup,
           .dwFlags = STARTF_USESTDHANDLES,
           .hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
           .hStdError = GetStdHandle(STD_INPUT_HANDLE),
    };
    PROCESS_INFORMATION pinfo{};
    const auto success = CreateProcessA(nullptr, //full_exe.c_str(),
                                        full_args.data(),
                                        nullptr, // sec attributes
                                        nullptr, // thread attributes
                                        true,    // Inherit handles
                                        INHERIT_PARENT_AFFINITY,
                                        nullptr, // env
                                        nullptr, // workdir
                                        &startup,
                                        &pinfo);
    if (!success) throw std::runtime_error(
           "failed starting(" + std::to_string(GetLastError()) + "): " + full_args);

    WaitForSingleObject(pinfo.hProcess, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeProcess(pinfo.hProcess, &exit_code);

    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    return exit_code;
}
