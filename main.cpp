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

#include <antlr4-runtime.h>
#include "TLexer.h"
#include "TParser.h"
#include "manager.hpp"

using namespace parsing;
using namespace antlr4;
using namespace std::string_literals;

int main(int argc, char *argv[]) {
    auto debug = false, quiet = false;
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
        if (*argv == "-d"s || *argv == "--debug"s)
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

    std::unique_ptr<CharStream> input;
    if (in.empty()) {
        input = std::make_unique<ANTLRInputStream>(std::cin);
    } else {
        auto tmp = std::make_unique<ANTLRFileStream>();
        tmp->loadFromFile(in);
        input = std::move(tmp);
    }

    TLexer lexer{ input.get() };
    CommonTokenStream tokens{ &lexer };
    tokens.fill();
    TParser parser{ &tokens };

    auto result = parser.main();
    if (parser.getNumberOfSyntaxErrors())
        return 1;

    manager mgr{ debug, quiet };
    result->accept(&mgr);

    if (out.empty()) {
        std::cout << mgr;
    } else {
        std::ofstream ofs{ out };
        ofs << mgr;
    }
}
