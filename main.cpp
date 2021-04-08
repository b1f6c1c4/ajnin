#include <iostream>

#include <antlr4-runtime.h>
#include "TLexer.h"
#include "TParser.h"
#include "manager.hpp"

using namespace parsing;
using namespace antlr4;
using namespace std::string_literals;

int main(int argc, char *argv[]) {
    auto debug = false;
    std::string in, out;

    argc--, argv++;
    for (; argc; argc--, argv++) {
        if (*argv == "-h"s || *argv == "--help"s) {
            std::cout << "Usage: ajnin [-h|--help] [-d|--debug] [-o <output>] [<file>]\n";
            return 0;
        }
        if (*argv == "-d"s || *argv == "--debug"s)
            debug = true;
        else if (*argv == "-o"s)
            out = argv[1];
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

    manager mgr{ debug };
    result->accept(&mgr);

    if (out.empty()) {
        std::cout << mgr;
    } else {
        std::ofstream ofs{ out };
        ofs << mgr;
    }
}
