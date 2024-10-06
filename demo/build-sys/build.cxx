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
 * demo/build-sys/build --
 *   
 */

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <lrt/linda.hxx>

#include "exec.hxx"

namespace fs = std::filesystem;
namespace stdv = std::views;
namespace stdr = std::ranges;

struct command {
    std::string type;
    std::string output;
    std::string inputs;

    void
    start() const {
        out(type, output, inputs);
    }
};

std::vector<command>
read_commands(std::istream& it) {
    std::vector<command> ret;

    command buf;
    while (std::getline(it >> buf.type >> buf.output >> std::ws, buf.inputs)) {
        ret.push_back(buf);
    }

    return ret;
}

int
execute_compiler(int id,
                 const fs::path& out,
                 const fs::path& input) {
    std::ostringstream ss;
    ss << "-c -ftemplate-depth=4000 -o " << out << " " << input;
//    std::ofstream("_build.log", std::ios::app) << "g++ " << ss.str() << "\n";
    int x = cross_exec("g++", ss.str());
//    std::ofstream("_build.log", std::ios::app) << "(" << lrt::this_runtime().rank() << "): CC" << id << ": g++ " << ss.str() << "... " << x << "\n";
    return x;
}

int
execute_linker(int id,
               const fs::path& out,
               const std::string& inputs) {
    std::ostringstream ss;
    ss << "-o " << out << " " << inputs;
    //    std::ofstream("_build.log", std::ios::app) << "(" << lrt::this_runtime().rank() << "): LINK" << id << ": g++ " << ss.str() << "...\n";
    auto x = cross_exec("g++", ss.str());
//    std::ofstream("_build.log", std::ios::app) << "(" << lrt::this_runtime().rank() << "): LINK" << id << ": g++ " << ss.str() << "... " << x << "\n";
    return x;
}

int
work_compiler(int worker_id) {
    for (;;) {
        command cmd;
        if (!inp("CC", ldb::ref(&cmd.output), ldb::ref(&cmd.inputs))) break;
        auto stat = execute_compiler(worker_id, cmd.output, cmd.inputs);
        if (stat != 0) throw std::runtime_error(std::format("fatal: {} failed with exit code: {}", cmd.output, stat));
        out(cmd.output);
    }
    out("_DONE", "CC", worker_id);
    return 0;
}

int
work_linker(int worker_id) {
    for (;;) {
        command cmd;
        if (!inp("LINK", ldb::ref(&cmd.output), ldb::ref(&cmd.inputs))) break;
        std::stringstream ss(cmd.inputs);
        stdr::for_each(std::istream_iterator<std::string>(ss),
                       std::istream_iterator<std::string>(),
                       [](const std::string& x) { rd(x); });

        auto stat = execute_linker(worker_id, cmd.output, cmd.inputs);
        if (stat != 0) throw std::runtime_error(std::format("fatal: {} failed with exit code: {}", cmd.output, stat));
    }
    //    std::ofstream("_term.log", std::ios::app) << "adios(" << lrt::this_runtime().rank() << "): LINK" << worker_id << "\n";
    out("_DONE", "LINK", worker_id);
    return 0;
}

void
run_workers(int count) {
    for (int const i : stdv::iota(0, count)) {
        eval((work_compiler) (i));
        eval((work_linker) (i));
    }
    for (int const i : stdv::iota(0, count)) {
        in("_DONE", "CC", i);
        in("_DONE", "LINK", i);
    }
}

int
real_main(int argc, char** argv) {
    auto build_file = std::ifstream("build.lb");
    const auto commands = read_commands(build_file);

    stdr::for_each(commands, std::mem_fn(&command::start));

    run_workers(lrt::this_runtime().world_size() - 1);

    return 0;
}
