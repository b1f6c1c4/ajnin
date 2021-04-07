#include <iostream>

#include <antlr4-runtime.h>
#include "TLexer.h"
#include "TParser.h"
#include "manager.hpp"

using namespace parsing;
using namespace antlr4;
using namespace std::string_literals;

int main(int , const char **) {
    ANTLRInputStream input{ std::cin };
    TLexer lexer(&input);
    CommonTokenStream tokens{ &lexer };
    tokens.fill();
    TParser parser{ &tokens };

    manager mgr{ true };
    parser.main()->accept(&mgr);

    return 0;
}
