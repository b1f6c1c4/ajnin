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

#include "manager.hpp"

#include <filesystem>
#include <iostream>
#include "TLexer.h"

using namespace parsing;
using namespace std::string_literals;

manager::manager(bool debug, bool quiet, size_t limit) : _debug{ debug }, _debug_limit{ limit }, _quiet{ quiet } { }

antlrcpp::Any manager::visitMain(TParser::MainContext *ctx) {
    ctx_guard next{ _current };
    visitChildren(ctx);
    return {};
}

antlrcpp::Any manager::visitProlog(TParser::PrologContext *ctx) {
    auto s0 = ctx->LiteralNL()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    _prolog.emplace_back(expand_env(s0));
    return {};
}

antlrcpp::Any manager::visitEpilog(TParser::EpilogContext *ctx) {
    auto s0 = ctx->LiteralNL()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    _epilog.emplace_back(expand_env(s0));
    return {};
}

antlrcpp::Any manager::visitFileStmt(TParser::FileStmtContext *ctx) {
    auto s0 = ctx->Path()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    auto [s, flag] = expand(s0);
    if (flag) throw std::runtime_error{ "Glob not allowed in " + s0 };

    load_file(s);

    return {};
}

void manager::parse(antlr4::CharStream &is) {
    using namespace antlr4;
    TLexer lexer{ &is };
    CommonTokenStream tokens{ &lexer };
    tokens.fill();
    TParser parser{ &tokens };
    auto res = parser.main();
    if (parser.getNumberOfSyntaxErrors())
        throw std::runtime_error{ "Syntax error detected." };
    res->accept(this);
}

void manager::load_stream(std::istream &is) {
    antlr4::ANTLRInputStream s{ is };
    ctx_guard next{ _current };
    _current->cwd = std::filesystem::current_path();
    parse(s);
}

void manager::load_file(const std::string &str) {
    antlr4::ANTLRFileStream s{};
    if (_debug)
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Loading file " << str << "\n";
    _depth++;
    s.loadFromFile(str);
    ctx_guard next{ _current };
    _current->cwd = str;
    _current->cwd = _current->cwd->parent_path().lexically_normal();
    parse(s);
    _depth--;
}

std::ostream &parsing::operator<<(std::ostream &os, const manager &mgr) {
    for (auto &t : mgr._prolog)
        os << manager::expand_dollar(t) << '\n';

    for (auto &[art, pb] : mgr._builds) {
        os << "build " << manager::expand_dollar(art) << ": " << manager::expand_dollar(pb->rule);
        for (auto &dep : pb->deps)
            os << " " << manager::expand_dollar(dep);
        if (!pb->ideps.empty()) {
            os << " |";
            for (auto &dep : pb->ideps)
                os << " " << manager::expand_dollar(dep);
        }
        if (!pb->vars.empty()) {
            os << '\n';
            for (auto &[va, vl] : pb->vars)
                os << "    " << manager::expand_dollar(va) << " = " << manager::expand_dollar(vl) << '\n';
        }
        os << '\n';
    }

    for (auto &t : mgr._epilog)
        os << manager::expand_dollar(t) << '\n';

    return os;
}
