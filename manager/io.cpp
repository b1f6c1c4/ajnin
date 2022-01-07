/* Copyright (C) 2021-2022 b1f6c1c4
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

#include <boost/regex.hpp>
#include <filesystem>
#include <iostream>
#include "TLexer.h"

using namespace parsing;
using namespace std::string_literals;

manager::manager(bool debug, bool quiet, size_t limit) : _debug{ debug }, _debug_limit{ limit }, _quiet{ quiet } { }

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

    _locations.push_front(ctx->getStart()->getInputStream()->getSourceName() +
                          ":" + std::to_string(ctx->KInclude()->getSymbol()->getLine()) +
                          ":" + std::to_string(ctx->Path()->getSymbol()->getStartIndex() + 1));
    load_file(s, true);
    _locations.pop_front();

    return {};
}

struct error_listener : antlr4::ANTLRErrorListener {
    const SS *prev_locations;

    void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol,
                     size_t line, size_t charPositionInLine, const std::string &msg,
                     std::exception_ptr e) override {
        for (auto &loc : *prev_locations) {
            if (&loc == &prev_locations->front())
                std::cerr << "In file included from ";
            else
                std::cerr << "                 from ";
            std::cerr << loc;
            if (&loc != &prev_locations->back())
                std::cerr << ",\n";
            else
                std::cerr << ":\n";
        }
        std::cerr << recognizer->getInputStream()->getSourceName();
        std::cerr << ":" << line << ":" << charPositionInLine + 1 << ": ";
        if (auto lexer = dynamic_cast<antlr4::Lexer *>(recognizer); lexer)
            std::cerr << "\e[31mlexical error\e[0m: ";
        if (auto parser = dynamic_cast<antlr4::Parser *>(recognizer); parser)
            std::cerr << "\e[31msyntax error\e[0m: ";
        std::cerr << msg << "\n";
    }
    void reportAmbiguity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex,
                         size_t stopIndex, bool exact, const antlrcpp::BitSet &ambigAlts,
                         antlr4::atn::ATNConfigSet *configs) override { }
    void reportAttemptingFullContext(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex,
                                     size_t stopIndex, const antlrcpp::BitSet &conflictingAlts,
                                     antlr4::atn::ATNConfigSet *configs) override { }
    void reportContextSensitivity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex,
                                  size_t stopIndex, size_t prediction,
                                  antlr4::atn::ATNConfigSet *configs) override { }
};

void manager::parse(antlr4::CharStream &is) {
    error_listener el{};
    el.prev_locations = &_locations;
    using namespace antlr4;
    TLexer lexer{ &is };
    lexer.removeErrorListeners();
    lexer.addErrorListener(&el);
    CommonTokenStream tokens{ &lexer };
    tokens.fill();
    TParser parser{ &tokens };
    parser.removeErrorListeners();
    parser.addErrorListener(&el);
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

void manager::load_file(const std::string &str, bool flat) {
    antlr4::ANTLRFileStream s{};
    if (_debug)
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Loading file " << str << "\n";
    _ajnin_deps.insert(str);
    _depth++;
    s.loadFromFile(str);
    if (flat) {
        auto old_cwd = std::move(_current->cwd);
        _current->cwd = str;
        _current->cwd = _current->cwd->parent_path().lexically_normal();
        parse(s);
        _current->cwd = std::move(old_cwd);
    } else {
        ctx_guard next{ _current };
        _current->cwd = str;
        _current->cwd = _current->cwd->parent_path().lexically_normal();
        parse(s);
    }
    _depth--;
}

static constexpr char g_ninja_prolog1[] = "# ajnin deps: ";
static constexpr char g_ninja_prolog2[] = "# No more ajnin deps.";

void manager::dump(std::ostream &os, const SS &slices, const SS &solos, bool bare) {
    if (!_quiet)
        std::cerr << "ajnin: Emitting " << _builds.size() << " builds\n";

    std::deque<boost::regex> the_slices, the_solos;
    for (auto &s : slices)
        the_slices.emplace_back(s);
    for (auto &s : solos)
        the_solos.emplace_back(s);

    S max_deps_art;
    size_t max_deps{};
    for (auto &[art, pb] : _builds) {
        pb->dedup();
        if (pb->deps.size() > max_deps) {
            max_deps_art = art;
            max_deps = pb->deps.size();
        }
    }
    if (!_quiet)
        std::cerr << "ajnin: Largest fanin is " << max_deps << " deps (" << max_deps_art << ")\n";

    if (!bare) {
        os << "# This file is automatically generated by ajnin. DO NOT MODIFY.\n";
        for (auto &d : _ajnin_deps)
            os << g_ninja_prolog1 << d << "\n";
        os << g_ninja_prolog2 << "\n";
    }

    for (auto &t : _prolog)
        os << manager::expand_dollar(t) << '\n';

    size_t cnt{};
    for (auto &[art, pb] : _builds) {
        auto the_art = manager::expand_dollar(art);
        auto ma = [&](const boost::regex &re) { return boost::regex_match(the_art, re); };
        if (!the_solos.empty() && std::none_of(the_solos.begin(), the_solos.end(), ma)) continue;
        if (!the_slices.empty() && std::any_of(the_slices.begin(), the_slices.end(), ma))
            if (std::filesystem::exists(the_art))
                continue;

        cnt++;
        os << "build " << the_art << ": " << manager::expand_dollar(pb->rule);
        for (auto &dep : pb->deps)
            os << " " << manager::expand_dollar(dep);
        if (!pb->ideps.empty()) {
            os << " |";
            for (auto &dep : pb->ideps)
                os << " " << manager::expand_dollar(dep);
        }
        if (!pb->iideps.empty()) {
            os << " ||";
            for (auto &dep : pb->iideps)
                os << " " << manager::expand_dollar(dep);
        }
        if (!pb->vars.empty() || _pools.contains(art)) {
            os << '\n';
            for (auto &[va, vl] : pb->vars)
                os << "    " << manager::expand_dollar(va) << " = " << manager::expand_dollar(vl) << '\n';
            if (_pools.contains(art))
                os << "    pool = " << _pools.at(art) << "\n";
        }
        os << '\n';
    }

    for (auto &t : _epilog)
        os << manager::expand_dollar(t) << '\n';

    if (!_quiet)
        std::cerr << "ajnin: Emitted " << cnt << " out of " << _builds.size() << " builds\n";
}

bool manager::collect_deps(const S &fn, bool debug) {
    Ss deps;

    {
        std::ifstream ifs{ fn };
        if (!ifs.good()) {
            if (debug)
                std::cerr << "ajnin: Notice: Output file does not exist\n";
            return false;
        }
        while (!ifs.eof()) {
            S s;
            std::getline(ifs, s);
            if (!ifs.good()) {
                std::cerr << "ajnin: Warning: Cannot parse output file\n";
                return false;
            }
            if (!s.starts_with('#')) {
                std::cerr << "ajnin: Warning: Output file does not have deps info\n";
                return false;
            }
            if (s.starts_with(g_ninja_prolog2)) break;
            if (s.starts_with(g_ninja_prolog1))
                deps.emplace(s.substr(sizeof(g_ninja_prolog1) - 1));
        }
    }

    auto mt = std::filesystem::last_write_time(fn);

    auto good = true;
    for (auto &dep : deps) {
        std::filesystem::path p{ dep };
        if (!std::filesystem::exists(p)) {
            if (debug)
                std::cerr << "ajnin: Info: meta-dep " << dep << " does not exist.\n";
            good = false;
        } else if (std::filesystem::last_write_time(p) > mt) {
            if (debug)
                std::cerr << "ajnin: Info: meta-dep " << dep << " has been updated.\n";
            good = false;
        } else if (debug)
            std::cerr << "ajnin: Info: meta-dep " << dep << " is up-to-date.\n";
        if (!debug && !good) break;
    }

    return good;
}

antlrcpp::Any manager::visitMetaStmt(TParser::MetaStmtContext *ctx) {
    for (auto &s : ctx->stage()) {
        s->accept(this);
        _ajnin_deps.emplace(std::move(_current_artifact));
    }
    return {};
}

void manager::split_dump(const S &out, const SS &slices, const SS &solos, const SS &eps, size_t par) {
    std::deque<boost::regex> the_slices, the_solos, the_eps;
    for (auto &s : slices)
        the_slices.emplace_back(s);
    for (auto &s : solos)
        the_solos.emplace_back(s);
    for (auto &s : eps)
        the_eps.emplace_back(s);
    auto is_ignored = [&](const S &the_art) {
        auto ma = [&](const boost::regex &re) { return boost::regex_match(the_art, re); };
        if (!the_solos.empty() && std::none_of(the_solos.begin(), the_solos.end(), ma)) return true;
        if (!the_slices.empty() && std::any_of(the_slices.begin(), the_slices.end(), ma))
            if (std::filesystem::exists(the_art))
                return true;
        return false;
    };

    // none: Unassigned
    // 0: Assigned to the common file
    // 1 ~ par: Assigned to a split file
    MS<size_t> assignment;
    SS queue;
    std::vector<size_t> cnts(par + 1);

    if (!_quiet)
        std::cerr << "ajnin: Finding endpoints from " << _builds.size() << " builds\n";

    // Initial round-robin assignment
    for (auto &[art, pb] : _builds) {
        auto the_art = manager::expand_dollar(art);
        if (is_ignored(the_art)) continue;
        for (auto &re : the_eps) {
            boost::smatch m;
            if (!boost::regex_match(the_art, m, re)) continue;
            auto s = m.size() >= 2 ? m[1] : m[0];
            cnts[assignment[art] = 1 + (std::hash<S>{}(s) % par)]++;
            queue.emplace_back(art);
        }
    }

    if (!_quiet)
        std::cerr << "ajnin: Spliting " << _builds.size() << " builds "
                  << "with " << queue.size() << " endpoints into " << par << " files, "
                  << "avg. " << queue.size() / par << " ep/file.\n";

    // Adjusting assignment
    while (!queue.empty()) {
        auto art = queue.front();
        queue.pop_front();
        auto pb = _builds.at(art);
        auto ass = assignment.at(art);

        auto fix = [&](const S &dep) {
            if (!_builds.contains(dep)) return;
            if (is_ignored(manager::expand_dollar(dep))) return;
            auto it = assignment.find(dep);
            if (it == assignment.end()) {
                assignment[dep] = ass;
                cnts[ass]++;
                queue.emplace_back(dep);
            } else if (!it->second || it->second == ass) {
                // do nothing
            } else {
                cnts[it->second]--;
                it->second = 0;
                cnts[0]++;
                queue.emplace_back(dep);
            }
        };

        for (auto &dep : pb->deps)
            fix(dep);
        for (auto &idep : pb->ideps)
            fix(idep);
        for (auto &iidep : pb->iideps)
            fix(iidep);
    }

    if (!_quiet) {
        auto rest = _builds.size();
        for (size_t i{}; i <= par; i++) {
            if (!i)
                std::cerr << "ajnin: File common has " << cnts[i] << " builds;\n";
            else
                std::cerr << "ajnin: File #" << i - 1 << " has " << cnts[i] << " builds;\n";
            rest -= cnts[i];
        }
        std::cerr << "ajnin: There are " << rest << " builds unassigned.\n";
    }

    std::vector<std::unique_ptr<std::ofstream>> ofss;
    ofss.reserve(1 + par);
    for (size_t i{}; i <= par; i++) {
        auto bd = out + "/";
        if (i) bd += std::to_string(i - 1) + "/";
        std::system(("mkdir -p "s + bd).c_str());
        auto pos = std::make_unique<std::ofstream>(bd + "build.ninja");
        if (i) *pos << "builddir = " << bd << "\n";
        ofss.emplace_back(std::move(pos));
    }

    for (auto &pos : ofss)
        for (auto &t : _prolog)
            *pos << manager::expand_dollar(t) << '\n';

    size_t cnt{};
    for (auto &[art, pb] : _builds) {
        auto it = assignment.find(art);
        if (it == assignment.end()) continue;

        auto the_art = manager::expand_dollar(art);

        cnt++;
        auto &os = *ofss[it->second];
        os << "build " << the_art << ": " << manager::expand_dollar(pb->rule);
        for (auto &dep : pb->deps)
            os << " " << manager::expand_dollar(dep);
        if (!pb->ideps.empty()) {
            os << " |";
            for (auto &dep : pb->ideps)
                os << " " << manager::expand_dollar(dep);
        }
        if (!pb->iideps.empty()) {
            os << " ||";
            for (auto &dep : pb->iideps)
                os << " " << manager::expand_dollar(dep);
        }
        if (!pb->vars.empty() || _pools.contains(art)) {
            os << '\n';
            for (auto &[va, vl] : pb->vars)
                os << "    " << manager::expand_dollar(va) << " = " << manager::expand_dollar(vl) << '\n';
            if (_pools.contains(art))
                os << "    pool = " << _pools.at(art) << "\n";
        }
        os << '\n';
    }

    for (auto &pos : ofss)
        for (auto &t : _epilog)
            *pos << manager::expand_dollar(t) << '\n';

    if (!_quiet)
        std::cerr << "ajnin: Emitted " << cnt << " out of " << _builds.size() << " builds\n";
}
