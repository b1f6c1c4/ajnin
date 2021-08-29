/* Copyright (C) 2021 b1f6c1c4
 *
 * This file is part of ajnin.
 *
 * ajnin is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, version 3.
 *
 * ajnin is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with ajnin.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <deque>
#include <ext/stdio_filebuf.h>

#include "config.h"
#include "manager.hpp"

using namespace std::string_literals;

void usage() {
    std::cout << "ajnin " PROJECT_VERSION "\n\n";
    std::cout << "Usage: ajnin [-h|--help] [-q|--quiet] [-C <chdir>] [-d|--debug] [-o <output>]\n";
    std::cout << "             [-s|--slice <regex>]... [-S|--solo <regex>]... [--bare]\n";
    std::cout << "             [<input>]\n";
    std::cout << "Note: -s and -S implies --bare, which cannot be override\n";
    std::cout << "\n";
    std::cout << "Usage: an    [-h|--help] [-q|--quiet] [-C <chdir>] [-o <build.ninja>]\n";
    std::cout << "             [-s|--slice <regex>]... [-S|--solo <regex>]... [--bare]\n";
    std::cout << "             [-f <build.ajnin>] [<ninja command line arguments>]...\n";
    std::cout << "Note: -s and -S implies -o '', but can be override\n";
    std::cout << "\n";
    std::cout << R"(
Copyright (C) 2021 b1f6c1c4

This file is part of ajnin.

ajnin is free software: you can redistribute it and/or modify it under the
terms of the GNU Affero General Public License as published by the Free
Software Foundation, version 3.

ajnin is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
more details.

You should have received a copy of the GNU Affero General Public License
along with ajnin.  If not, see <https://www.gnu.org/licenses/>.
)";
}

int main(int argc, char *argv[]) {
    auto debug = false, quiet = false, bare = false;
    std::string in, out;
    std::deque<std::string> slices, solos;
    std::vector<const char *> ninja_args{ "ninja" };

    bool ninja;
    if (std::string_view{ *argv }.ends_with("ajnin"))
        ninja = false;
    else if (std::string_view{ *argv }.ends_with("an"))
        ninja = true, out = "build.ninja", in = "build.ajnin";
    else {
        std::cerr << "Unknown argv[0]: " << argv[0] << std::endl;
        usage();
        return 0;
    }
    argc--, argv++;
    for (; argc; argc--, argv++) {
        if (*argv == "-h"s || *argv == "--help"s || *argv == "--version"s) {
            usage();
            return 0;
        }
        if (*argv == "--bare"s)
            bare = true;
        else if (!ninja && (*argv == "-d"s || *argv == "--debug"s))
            debug = true;
        else if (*argv == "-q"s || *argv == "--quiet"s)
            quiet = true;
        else if (*argv == "-o"s)
            out = argv[1], argc--, argv++;
        else if (*argv == "-C"s)
            chdir(argv[1]), argc--, argv++;
        else if (*argv == "-s"s || *argv == "--slice"s) {
            slices.emplace_back(argv[1]), argc--, argv++;
            bare = true;
            if (ninja)
                out = "";
        } else if (*argv == "-S"s || *argv == "--solo"s) {
            solos.emplace_back(argv[1]), argc--, argv++;
            bare = true;
            if (ninja)
                out = "";
        } else if (ninja && *argv == "-f"s)
            in = argv[1], argc--, argv++;
        else if (ninja)
            ninja_args.push_back(argv[0]);
        else if (!in.empty())
            throw std::runtime_error{ "Too many input files" };
        else
            in = argv[0];
    }

    auto execute = [&](std::ostream &os) {
        parsing::manager mgr{ debug, quiet };
        if (in.empty()) {
            mgr.load_stream(std::cin);
        } else {
            mgr.load_file(in);
        }
        mgr.dump(os, slices, solos, bare);
    };

    if (out.empty()) {
        if (ninja) {
            ninja_args.push_back("-f");
            ninja_args.push_back("/dev/stdin");
            ninja_args.push_back(nullptr);
            int fds[2];
            if (pipe(fds) == -1)
                throw std::runtime_error{ "Cannot make a named pipe" };
            int child;
            if ((child = fork()) == -1)
                throw std::runtime_error{ "Cannot fork" };
            if (child) { // I'm the parent
                while ((dup2(fds[0], STDIN_FILENO) == -1) && (errno == EINTR));
                close(fds[0]);
                close(fds[1]);
                execvp("ninja", const_cast<char *const *>(ninja_args.data()));
                return 127;
            }
            // I'm the child
            close(fds[0]);
            __gnu_cxx::stdio_filebuf<char> fb(fds[1], std::ios::out);
            std::ostream os{ &fb };
            execute(os);
            return 0;
        } else { // Write to stdout
            execute(std::cout);
            return 0;
        }
    } else {
        if (!parsing::manager::collect_deps(out, debug)) {
            std::ofstream ofs{ out };
            execute(ofs);
        }
        if (ninja) {
            ninja_args.push_back("-f");
            ninja_args.push_back(out.data());
            ninja_args.push_back(nullptr);
            execvp("ninja", const_cast<char *const *>(ninja_args.data()));
        }
    }
}
