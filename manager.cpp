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

rule_t &rule_t::operator+=(const rule_t &o) {
    if (!name.empty() && !o.name.empty() && name != o.name)
        throw std::runtime_error{ "Cannot add rules under different names" };
    if (!o.name.empty())
        name = o.name;
    for (auto &[k, v] : o.vars)
        vars[k] = v;
    for (auto &dep : o.ideps)
        ideps.insert(dep);
    return *this;
}

list_item_t *manager::ctx_t::operator[](const C &s) const {
    if (ass.contains(s)) return ass.at(s);
    if (!prev) return {};
    return prev->operator[](s);
}

rule_t manager::ctx_t::operator[](const S &s) const {
    auto r = prev ? prev->operator[](s) : rule_t{ s };
    r += zrule;
    if (rules.contains(s)) r += rules.at(s);
    return r;
}

pbuild_t manager::ctx_t::make_build() const {
    auto pb = prev ? prev->make_build() : std::make_shared<build_t>();
    for (auto &dep : ideps)
        pb->ideps.insert(dep);
    return pb;
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

S expand_quote(S s, char c) {
    for (size_t i{}; i < s.size(); i++) {
        if (s[i] != '$') continue;
        if (i == s.size() - 1) continue;
        if (s[i + 1] == c)
            i++;
    }
    return s;
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

manager::manager(bool debug, size_t limit) : _debug{ debug }, _debug_limit{ limit } { }

antlrcpp::Any manager::visitMain(TParser::MainContext *ctx) {
    ctx_t next{ _current };
    _current = &next;
    visitChildren(ctx);
    _current = next.prev;
    return {};
}

antlrcpp::Any manager::visitRuleStmt(TParser::RuleStmtContext *ctx) {
    SS rules;
    for (auto t : ctx->Token())
        rules.emplace_back(t->getText());

    if (rules.empty()) {
        _current_rule = &_current->zrule;
        visitChildren(ctx);
        _current_rule = nullptr;
        _current_artifact.clear();
        return {};
    }

    for (auto &rule : rules) {
        _current_rule = &_current->rules[rule];
        _current_rule->name = rule;
        visitChildren(ctx);
        _current_rule = nullptr;
        _current_artifact.clear();
    }
    return {};
}

antlrcpp::Any manager::visitGroupStmt(TParser::GroupStmtContext *ctx) {
    ctx_t next{ _current };
    _current = &next;

    auto ids = ctx->ID();

    if (_debug) {
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Entering group of";
        for (auto id : ids)
            std::cerr << ' ' << as_id(id);
        std::cerr << '\n';
    }
    _depth++;

    if (ids.empty()) {
        visitChildren(ctx);
    } else {
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
                _current->zrule = rule_t{};
                _current->rules.clear();
                _current->ideps.clear();
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
    auto &s = _current_list->items;
    if (ctx->ListEnumItem())
        s.emplace_back(std::move(item));
    else if (ctx->ListEnumRItem())
        s.erase(std::remove_if(s.begin(), s.end(), [&](const list_item_t &it) {
            if (it.name != item.name) return false;
            return std::equal(it.args.begin(), it.args.end(), item.args.begin());
        }), s.end());
    return {};
}

antlrcpp::Any manager::visitListInlineEnumStmt(TParser::ListInlineEnumStmtContext *ctx) {
    for (auto t : ctx->ListItemToken())
        _current_list->items.emplace_back(list_item_t{ expand_env(t->getText()) });
    return {};
}

antlrcpp::Any manager::visitPipeStmt(TParser::PipeStmtContext *ctx) {
    _current_build = nullptr;
    ctx->pipe()->accept(this);
    _current_build = nullptr;
    return {};
}

// _current_build must be valid.
// _current_build.i?dep will be appended.
// _current_artifact will be cleared.
antlrcpp::Any manager::visitPipeGroup(TParser::PipeGroupContext *ctx) {
    _current_artifact.clear();
    visitChildren(ctx);
    _current_artifact.clear();
    return {};
}

// _current_build must be valid.
// _current_build.i?dep will be appended once.
// _current_artifact will be set.
antlrcpp::Any manager::visitArtifact(TParser::ArtifactContext *ctx) {
    visitChildren(ctx);
    if (!ctx->Tilde())
        _current_build->deps.emplace_back(_current_artifact);
    else
        _current_build->ideps.insert(_current_artifact);
    return {};
}

// _current_build will not be used nor set.
// _current_artifact will be set.
antlrcpp::Any manager::visitPipe(TParser::PipeContext *ctx) {
    auto prev = std::move(_current_build);
    _current_build = _current->make_build();
    visitChildren(ctx);
    _current_build = std::move(prev);
    return {};
}

// _current_artifact will be set.
antlrcpp::Any manager::visitStage(TParser::StageContext *ctx) {
    auto s0 = ctx->Stage()->getText();
    if (!s0.starts_with('(') || !s0.ends_with(')')) throw std::runtime_error{ "Lexer messed up with ()" };
    s0 = s0.substr(1, s0.length() - 2);

    auto [s, glob] = expand(s0);
    if (glob) throw std::runtime_error{ "Glob not allow in " + s0 };
    if (_current_rule)
        _current_rule->ideps.insert(s);
    _current_artifact = std::move(s);
    return {};
}

// _current_build must be valid.
// _current_build will be replaced by a new empty value.
// _current_artifact will be set.
antlrcpp::Any manager::visitOperation(TParser::OperationContext *ctx) {
    // This handles the case when operation follows immediately after stage
    if (_current_build->deps.empty() && _current_build->ideps.empty())
        _current_build->deps.emplace_back(std::move(_current_artifact));

    rule_t rule{};
    _current_rule = &rule;
    if (!ctx->Token()) {
        _current_build->rule = "phony";
    } else {
        _current_build->rule = ctx->Token()->getText();
        *_current_rule = (*_current)[_current_build->rule];
        for (auto ass : ctx->assignment())
            ass->accept(this);
    }
    _current_rule = nullptr;
    for (auto &[k, v] : rule.vars)
        _current_build->vars[k] = v;
    for (auto &dep : rule.ideps)
        _current_build->ideps.insert(dep);

    ctx->stage()->accept(this);
    _current_build->art = _current_artifact;

    auto &pb = _builds[_current_artifact];
    if (!pb) pb = std::make_shared<build_t>();
    *pb += std::move(*_current_build);

    _current_build = std::make_shared<build_t>();
    return {};
}

antlrcpp::Any manager::visitAssignment(TParser::AssignmentContext *ctx) {
    auto as = ctx->Assign()->getText();
    if (!as.starts_with('&') || !as.ends_with('=')) throw std::runtime_error{ "Lexer messed up with &=" };
    auto append = as[as.size() - 2] == '+';
    as = as.substr(1, as.length() - (append ? 3 : 2));

    S str;
    if (ctx->ID()) {
        auto id = ctx->ID()->getText();
        if (id.size() != 1) throw std::runtime_error{ "Lexer messed up with ID" };
        auto a = (*_current)[id[0]];
        if (!a) return {};
        str = a->name;
    } else if (ctx->SubID()) {
        auto id = ctx->SubID()->getText();
        if (id.size() != 2) throw std::runtime_error{ "Lexer messed up with SubID" };
        auto a = (*_current)[id[0]];
        if (!a) return {};
        auto v = id[1] - '0';
        if (v >= a->args.size())
            return {};
        str = a->args[v];
    } else if (ctx->SingleString()) {
        auto s0 = ctx->SingleString()->getText();
        if (!s0.starts_with('\'') || !s0.ends_with('\'')) throw std::runtime_error{ "Lexer messed up with ''" };
        s0 = s0.substr(1, s0.size() - 2);
        s0 = expand_quote(s0, '\'');
        s0 = expand_env(s0);
        str = std::move(s0);
    } else if (ctx->DoubleString()) {
        auto s0 = ctx->DoubleString()->getText();
        if (!s0.starts_with('"') || !s0.ends_with('"')) throw std::runtime_error{ "Lexer messed up with \"\"" };
        s0 = s0.substr(1, s0.size() - 2);
        s0 = expand_quote(s0, '"');
        auto [s, glob] = expand(s0);
        if (glob) throw std::runtime_error{ "Glob not allow in " + s0 };
        str = std::move(s);
    } else
        throw std::runtime_error{ "Parser messed up in assignment" };

    auto &rule = _current_rule->name;
    if (append)
        str = (*_current)[rule].vars[as] + str;

    auto &dest = _current_rule->vars;
    dest[as] = str;
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
        if (!pb->ideps.empty()) {
            os << " |";
            for (auto &dep : pb->ideps)
                os << " " << dep;
        }
        if (!pb->vars.empty()) {
            os << '\n';
            for (auto &[va, vl] : pb->vars)
                os << "    " << va << " = " << vl << '\n';
        }
        os << '\n';
    }

    for (auto &t : mgr._epilog)
        os << t << '\n';

    return os;
}
