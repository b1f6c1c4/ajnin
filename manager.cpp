#include "manager.hpp"

#include <filesystem>
#include <iostream>
#include <stack>

using namespace parsing;
using namespace std::string_literals;

C as_id(antlr4::tree::TerminalNode *s) {
    return s->getText()[0];
}

S manager::ctx_t::operator[](const C &s) {
    if (ass.contains(s)) return ass[s];
    if (!prev) return {};
    return prev->operator[](s);
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
        const auto &li = _lists[c];
        if (li.items.empty()) return {};

        next.ass[c] = li.items[ii.top()].name;
        if (ii.size() == ids.size()) {
            if (_debug) {
                std::cerr << std::string(_depth * 2, ' ') << "ajnin:";
                for (auto id : ids)
                    std::cerr << " $" << as_id(id) << "=" << next.ass[as_id(id)];
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
    if (s0.ends_with('\n')) s0.pop_back();
    auto s = s0;
    S cur;
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
        if (a.empty()) throw std::runtime_error{ "List "s + a[i + 1] + " not enumerated yet"};
        s.replace(i, 2, a);
        i += a.size() - 1;
    }
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
    for (auto t : ctx->ListItemToken())
        if (!flag)
            item.name = t->getText(), flag = true;
        else
            item.args.emplace_back(t->getText());
    _current_list->items.emplace_back(std::move(item));
    return {};
}

antlrcpp::Any manager::visitListInlineEnumStmt(TParser::ListInlineEnumStmtContext *ctx) {
    for (auto t : ctx->ListItemToken())
        _current_list->items.emplace_back(list_item_t{ t->getText() });
    return {};
}

antlrcpp::Any manager::visitPipeStmt(TParser::PipeStmtContext *ctx) {
    // TODO: pipe
    for (auto &[c, l] : _lists) {
        std::cout << c << " ->";
        for (auto &it : l.items)
            std::cout << " " << it.name;
        std::cout << "\n";
    }
    return {};
}

antlrcpp::Any manager::visitRuleStmt(TParser::RuleStmtContext *ctx) {
    for (auto p : ctx->Path())
        _rules[ctx->Token()->getText()].deps.emplace_back(p->getText());
    return {};
}
