#include "manager.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stack>

using namespace parsing;
using namespace std::string_literals;

C as_id(antlr4::tree::TerminalNode *s) {
    return s->getText()[0];
}

list_item_t *manager::ctx_t::operator[](const C &s) {
    if (ass.contains(s)) return ass[s];
    if (!prev) return {};
    return prev->operator[](s);
}

build_t &build_t::operator+=(build_t &&o) {
    if (art.empty())
        return *this = std::move(o);

    if (rule != o.rule)
        throw std::runtime_error{ "Conflict rule for " + art };
    if (!std::equal(vars.begin(), vars.end(), o.vars.begin()))
        throw std::runtime_error{ "Conflict var for " + art };

    for (auto &dep : o.deps)
        deps.emplace_back(std::move(dep));
    o.deps.clear();
    return *this;
}

S manager::expand_env(const S &s0) const {
    auto s = s0;
    for (size_t i{}; i < s.size(); i++) {
        if (s[i] != '$') continue;
        if (i == s.size() - 1) throw std::runtime_error{ "Invalid string " + s0 };
        switch (s[i + 1]) {
            case '$':
            default:
                i++;
                continue;
            case '{':
                break;
        }

        auto e = s.find('}', i + 2);
        if (e == std::string::npos) throw std::runtime_error{ "Invalid string " + s0 };
        auto env = s.substr(i + 2, e - i - 2);
        const char *st = std::getenv(env.c_str());
        if (!st) {
            if (_env_notif.insert(env).second)
                std::cerr << "ajnin: Warning: Environment variable ${" << env << "} not found.\n";
            st = "";
        }
        s.replace(i, e - i + 1, st);
        i = e + 1;
    }
    return s;
}

std::pair<S, bool> manager::expand(const S &s0) const {
    auto s = expand_env(s0);
    auto flag = false;
    for (size_t i{}; i < s.size(); i++) {
        if (s[i] != '$') continue;
        if (i == s.size() - 1) throw std::runtime_error{ "Invalid path " + s0 };
        if (s[i + 1] == '$') {
            if (flag) throw std::runtime_error{ "Multiple globs in " + s0 };
            flag = true;
            i++;
            continue;
        }
        auto a = (*_current)[s[i + 1]];
        if (!a) throw std::runtime_error{ "List "s + s[i + 1] + " not enumerated yet"};
        auto st = [&]() {
            if (i != s.size() - 2 && std::isdigit(s[i + 2])) {
                auto v = s[i + 2] - '0';
                if (v >= a->args.size())
                    return ""s;
                return a->args[v];
            }
            return a->name;
        }();
        s.replace(i, 2, st);
        i += st.size() - 1;
    }
    return { s, flag };
}

antlrcpp::Any manager::visitGroupStmt(TParser::GroupStmtContext *ctx) {
    ctx_t next{ _current };
    _current = &next;

    auto ids = ctx->ID();
    if (ids.empty()) throw std::runtime_error{ "Empty group list of lists." };

    if (_debug) {
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Entering group of";
        for (auto id : ids)
            std::cerr << ' ' << as_id(id);
        std::cerr << '\n';
    }
    _depth++;

    std::stack<size_t> ii;
    ii.push(0);
    while (true) {
        auto c = as_id(ids[ii.size() - 1]);
        auto &li = _lists[c];
        if (li.items.empty()) return {};

        next.ass[c] = &li.items[ii.top()];
        if (ii.size() == ids.size()) {
            if (_debug) {
                std::cerr << std::string(_depth * 2, ' ') << "ajnin:";
                for (auto id : ids)
                    std::cerr << " $" << as_id(id) << "=" << next.ass[as_id(id)]->name;
                std::cerr << '\n';
            }
            visitChildren(ctx);
            ii.top()++;
        }

        if (ii.top() == li.items.size()) {
            ii.pop();
            if (ii.empty())
                break;
            ii.top()++;
            continue;
        }

        if (ii.size() < ids.size())
            ii.push(0);
    }

    _depth--;
    if (_debug) {
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Exiting group of";
        for (auto id : ids)
            std::cerr << ' ' << as_id(id);
        std::cerr << '\n';
    }

    _current = next.prev;
    return {};
}

antlrcpp::Any manager::visitListStmt(TParser::ListStmtContext *ctx) {
    auto c = as_id(ctx->ID());
    _current_list = &_lists[c];
    if (_debug) {
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Setting list " << c << '\n';
    }
    _depth++;
    visitChildren(ctx);
    _depth--;
    if (_debug) {
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: List " << c
                  << " now is (" << _current_list->items.size() << " items):\n";
        size_t cnt{};
        for (auto &it : _current_list->items) {
            std::cerr << std::string(_depth * 2, ' ') << "           ";
            if (_debug_limit && cnt++ == _debug_limit) {
                std::cerr << "...\n";
                break;
            }
            std::cerr << it.name;
            if (!it.args.empty()) {
                std::cerr << "[";
                auto flag = false;
                for (auto &f : it.args) {
                    if (flag) std::cerr << ",";
                    flag = true;
                    std::cerr << f;
                }
                std::cerr << ']';
            }
            std::cerr << '\n';
        }
    }
    _current_list = nullptr;
    return {};
}

using P = std::filesystem::path;
using PC = P::const_iterator;
using DI = std::filesystem::directory_iterator;

struct glob_t {
    S l, r;
    bool exact;
    enum ans_t {
        REJECT,
        ACCEPT,
        MATCH,
    };
    explicit glob_t(const S &str) {
        auto pos = str.find("$$");
        if (pos == std::string::npos) { // filename may NOT have glob.
            l = str, r = {}, exact = true;
            return;
        }
        l = str.substr(0, pos);
        r = str.substr(pos + 2);
        exact = false;
    }
    [[nodiscard]] std::pair<ans_t, std::string> match(const S &str) const {
        if (!str.starts_with(l)) return { REJECT, {} };
        if (exact)
            return { l.length() + r.length() > str.length() ? REJECT : ACCEPT, {} };
        if (l.length() + r.length() >= str.length()) return { REJECT, {} };
        if (!str.ends_with(r)) return { REJECT, {} };
        return { MATCH, str.substr(l.length(), str.length() - l.length() - r.length()) };
    }
};

// directory: actual dir that's being searched
// start: head of pattern
// finish: tail of pattern
// filename: very tail of pattern
void glob_search(P directory, PC start, const PC &finish, const S &filename, std::function<bool(const S &)> cb) {
    // proceed if there is no glob
    while (start != finish && start->string().find("$$") == std::string::npos)
        directory /= *start++;

    DI it{ directory, std::filesystem::directory_options::skip_permission_denied };
    if (it == DI{}) return;

    if (start == finish) {
        glob_t glob{ filename };
        do {
            if (it->is_directory()) continue;
            if (auto [r, s] = glob.match(it->path().filename().string()); r != glob_t::REJECT)
                if (cb(s)) return;
        } while (++it != std::filesystem::directory_iterator());
    } else {
        glob_t glob{ start->string() };
        do {
            if (!it->is_directory()) continue;
            auto [r, s] = glob.match(std::prev(it->path().end())->string());
            if (r == glob_t::REJECT) continue;
            if (r == glob_t::ACCEPT)
                glob_search(it->path(), std::next(start), finish, filename, cb);
            else
                glob_search(it->path(), std::next(start), finish, filename, [&](const S &){ cb(s); return true; });
        } while (++it != std::filesystem::directory_iterator());
    }
}

antlrcpp::Any manager::visitListSearchStmt(TParser::ListSearchStmtContext *ctx) {
    auto s0 = ctx->Path()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    auto [s, flag] = expand(s0);
    if (!flag) throw std::runtime_error{ "No glob in " + s0 };

    std::filesystem::path p{ s };
    auto ins = [&](const P &pa) {
        _current_list->items.emplace_back(list_item_t{ pa.string() });
        return false;
    };
    if (p.is_absolute()) {
        auto rp = p.parent_path().relative_path();
        glob_search(p.root_path(), std::begin(rp), std::end(rp), p.filename().string(), ins);
    } else {
        auto pa = p.parent_path();
        glob_search(std::filesystem::current_path(), std::begin(pa), std::end(pa), p.filename().string(), ins);
    }
    return {};
}

antlrcpp::Any manager::visitListEnumStmtItem(TParser::ListEnumStmtItemContext *ctx) {
    list_item_t item;
    auto flag = false;
    for (auto t : ctx->ListItemToken()) {
        auto st = expand_env(t->getText());
        if (!flag)
            item.name = std::move(st), flag = true;
        else
            item.args.emplace_back(std::move(st));
    }
    _current_list->items.emplace_back(std::move(item));
    return {};
}

antlrcpp::Any manager::visitListInlineEnumStmt(TParser::ListInlineEnumStmtContext *ctx) {
    for (auto t : ctx->ListItemToken())
        _current_list->items.emplace_back(list_item_t{ expand_env(t->getText()) });
    return {};
}

antlrcpp::Any manager::visitPipeStmt(TParser::PipeStmtContext *ctx) {
    _current_build = std::make_shared<build_t>();

    ctx->stage()->accept(this);
    for (auto op : ctx->operation())
        op->accept(this);

    _current_build = nullptr;
    return {};
}

antlrcpp::Any manager::visitStage(TParser::StageContext *ctx) {
    auto s0 = ctx->Stage()->getText();
    if (!s0.starts_with('(') || !s0.ends_with(')')) throw std::runtime_error{ "Lexer messed up with ()" };
    s0 = s0.substr(1, s0.length() - 2);

    auto [s, flag] = expand(s0);
    if (flag) throw std::runtime_error{ "Glob not allow in " + s0 };
    _current_build->deps.emplace_back(std::move(s));
    return {};
}

antlrcpp::Any manager::visitOperation(TParser::OperationContext *ctx) {
    if (!ctx->Token()) {
        _current_build->rule = "phony";
    } else {
        _current_build->rule = ctx->Token()->getText();
        for (auto ass : ctx->assignment())
            ass->accept(this);
    }

    auto prev = std::move(_current_build);
    _current_build = std::make_shared<build_t>();
    ctx->stage()->accept(this);
    prev->art = _current_build->deps.front();
    auto &pb = _builds[prev->art];
    if (!pb) pb = std::make_shared<build_t>();
    *pb += std::move(*prev);
    return {};
}

antlrcpp::Any manager::visitAssignment(TParser::AssignmentContext *ctx) {
    auto as = ctx->Assign()->getText();
    if (!as.starts_with('$') || !as.ends_with('=')) throw std::runtime_error{ "Lexer messed up with $=" };
    as = as.substr(0, as.length() - 2);

    auto id = ctx->ID() ? ctx->ID()->getText() : ctx->SubID()->getText();
    if (id.empty() || id.size() > 2) throw std::runtime_error{ "Lexer messed up" };
    auto a = (*_current)[id[0]];
    if (!a) return {};

    if (id.size() == 1) {
        _current_build->vars[as] = a->name;
        return {};
    }

    auto v = id[1] - '0';
    if (v >= a->args.size())
        return {};
    _current_build->vars[as] = a->args[v];
    return {};
}

antlrcpp::Any manager::visitRuleStmt(TParser::RuleStmtContext *ctx) {
    auto s0 = ctx->Path()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    _rules[ctx->Token()->getText()].deps.emplace_back(expand_env(s0));
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

std::ostream &parsing::operator<<(std::ostream &os, const manager &mgr) {
    for (auto &t : mgr._prolog)
        os << t << '\n';

    for (auto &[art, pb] : mgr._builds) {
        os << "build " << art << ": " << pb->rule;
        for (auto &dep : pb->deps)
            os << " " << dep;
        if (mgr._rules.contains(pb->rule)) {
            auto &r = mgr._rules.at(pb->rule);
            if (!r.deps.empty()) {
                os << " |";
                for (auto &dep : r.deps)
                    os << " " << dep;
            }
        }
        if (!pb->vars.empty())
            for (auto &[va, vl] : pb->vars)
                os << "    " << va << " = " << vl << '\n';
        os << '\n';
    }

    for (auto &t : mgr._epilog)
        os << t << '\n';

    return os;
}
