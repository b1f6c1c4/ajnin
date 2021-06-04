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

#include "manager.hpp"

using namespace std::string_literals;

int main(int argc, char *argv[]) {
    auto debug = false, quiet = false, bare = false;
    std::string in, out;

    argc--, argv++;
    for (; argc; argc--, argv++) {
        if (*argv == "-h"s || *argv == "--help"s) {
            std::cout << "Usage: ajnin [-h|--help] [-d|--debug] [-q|--quiet] [-o <output>] [<file>]\n";
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
            return 0;
        }
        if (*argv == "--bare"s)
            bare = true;
        else if (*argv == "-d"s || *argv == "--debug"s)
            debug = true;
        else if (*argv == "-q"s || *argv == "--quiet"s)
            quiet = true;
        else if (*argv == "-o"s)
            out = argv[1], argc--, argv++;
        else if (!in.empty())
            throw std::runtime_error{ "Too many input files" };
        else
            in = argv[0];
    }

    if (!out.empty())
        if (parsing::manager::collect_deps(out, debug))
            return 0;

    parsing::manager mgr{ debug, quiet };

    if (in.empty()) {
        mgr.load_stream(std::cin);
    } else {
        mgr.load_file(in);
    }

    if (out.empty()) {
        mgr.dump(std::cout, bare);
    } else {
        std::ofstream ofs{ out };
        mgr.dump(ofs, bare);
    }
}
