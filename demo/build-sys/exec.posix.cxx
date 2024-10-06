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

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

namespace {
    std::optional<std::string>
    find_in_dir(const std::string& name, const std::filesystem::path& dir) {
        if (!exists(dir)) return {};

        std::filesystem::directory_iterator begin(dir);
        std::filesystem::directory_iterator end;
        auto it = std::find_if(begin, end, [&](const std::filesystem::directory_entry& entry) {
            if (!entry.is_regular_file() && !entry.is_symlink()) return false;
            return entry.path().filename() == name;
        });
        if (it == end) return {};
        return it->path().string();
    }

    std::string
    get_exec(const std::string& name) {
        const char* path = getenv("PATH");
        if (!path) path = "/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin";
        std::string_view path_sv(path);

        std::size_t last = 0;
        std::size_t pos = 0;
        while ((pos = path_sv.find(':', pos)) != path_sv.npos) {
            const auto path_entry = path_sv.substr(last, pos - last);

            if (auto found = find_in_dir(name, path_entry);
                found) {
                return *found;
            }

            ++pos;
            last = pos;
        }

        return name;
    }
}

int
cross_exec(const std::string& exe, const std::string& args) {
    wordexp_t words;
    words.we_offs = 1;
    wordexp(args.c_str(), &words, WRDE_NOCMD | WRDE_DOOFFS);

    auto full_exe = get_exec(exe);
    words.we_wordv[0] = full_exe.data();

    const auto child_pid = fork();
    if (!child_pid) {
        execve(full_exe.c_str(), words.we_wordv, __environ);
        std::cout << "execve(" << std::quoted(full_exe) << ", " << std::quoted(full_exe + " " + args) << "): "
                  << errno
                  << ": " << strerror(errno)
                  << "\n";
        exit(-10);
    }

    int stat;
    waitpid(child_pid, &stat, 0);

    wordfree(&words);

    if (WIFEXITED(stat)) return WEXITSTATUS(stat);
    return WTERMSIG(stat);
}
