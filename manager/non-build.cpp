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

#include <cstring>
#include <iostream>
#include <stack>
#include "TLexer.h"

using namespace parsing;
using namespace std::string_literals;

antlrcpp::Any manager::visitDebugStmt(TParser::DebugStmtContext *ctx) {
    if (_quiet) return {};

    if (auto c = as_id(ctx->ID()); c) {
        if (!_lists.contains(c)) {
            std::cerr << std::string(_depth * 2, ' ') << "ajnin: List " << c
                      << " does not exist\n";
        } else {
            const auto &list = _lists.at(c);
            std::cerr << std::string(_depth * 2, ' ') << "ajnin: List " << list.name
                      << " now is (" << list.items.size() << " items):\n";
            size_t cnt{};
            for (auto &it : list.items) {
                std::cerr << std::string(_depth * 2, ' ') << "           ";
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
    }

    return {};
}

antlrcpp::Any manager::visitClearStmt(TParser::ClearStmtContext *ctx) {
    if (auto c = as_id(ctx->ID()); c)
        _lists.erase(c);
    return {};
}

antlrcpp::Any manager::visitIfStmt(TParser::IfStmtContext *ctx) {
    auto decision = [&]() {
        auto id = ctx->SubID()->getText();
        if (id.size() != 2) throw std::runtime_error{ "Lexer messed up with SubID" };
        auto a = (*_current)[id[0]];
        if (!a) return false;
        auto v = id[1] - '0';
        if (v >= a->args.size())
            return false;
        return !a->args[v].empty();
    }() ^ !ctx->IsNonEmpty();

    if (decision) {
        ctx->stmts(0)->accept(this);
        return {};
    }

    if (ctx->ifStmt())
        ctx->ifStmt()->accept(this);
    else if (ctx->stmts(1))
        ctx->stmts(1)->accept(this);

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

antlrcpp::Any manager::visitForeachGroupStmt(TParser::ForeachGroupStmtContext *ctx) {
    ctx_guard next{ _current };

    auto ids = ctx->ID();

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

        _current->ass[c] = &li.items[ii.top()];
        if (ii.size() == ids.size()) {
            if (_debug) {
                std::cerr << std::string(_depth * 2, ' ') << "ajnin:";
                for (auto id : ids)
                    std::cerr << " $" << as_id(id) << "=" << _current->ass[as_id(id)]->name;
                std::cerr << '\n';
            }
            ctx->stmts()->accept(this);
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

    _depth--;
    if (_debug) {
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Exiting group of";
        for (auto id : ids)
            std::cerr << ' ' << as_id(id);
        std::cerr << '\n';
    }

    return {};
}

antlrcpp::Any manager::visitCollectGroupStmt(TParser::CollectGroupStmtContext *ctx) {
    ctx_guard next{ _current };

    _current->app_also = ctx->KAlso();
    _current->app = _current->make_build();

    visitChildren(ctx);
    return {};
}

antlrcpp::Any manager::visitCollectOperation(TParser::CollectOperationContext *ctx) {
    rule_t rule{};
    _current_rule = &rule;
    if (!ctx->Token()) {
        _current->app->rule = "phony";
    } else {
        _current->app->rule = ctx->Token()->getText();
        *_current_rule = (*_current)[_current->app->rule];
        for (auto ass : ctx->assignment())
            ass->accept(this);
    }
    _current_rule = nullptr;
    for (auto &[k, v] : rule.vars)
        _current->app->vars[k] = v;
    for (auto &dep : rule.ideps)
        _current->app->ideps.insert(dep);

    ctx->stage()->accept(this);
    _current->app->art = std::move(_current_artifact);
    _current_artifact = {};
    return {};
}

antlrcpp::Any manager::visitListGroupStmt(TParser::ListGroupStmtContext *ctx) {
    auto c = as_id(ctx->ID());
    if (_lists.contains(c))
        throw std::runtime_error{ "List "s + c + " already exists" };

    auto s0 = ctx->OpenCurlyPath()->getText();
    if (!s0.ends_with(" {\n")) throw std::runtime_error{ "Lexer messed up with  {\\n" };
    s0.erase(s0.end() - 3, s0.end());

    _current_list = &_lists[c];
    _current_list->name = c;

    list_search(s0);

    ctx_guard next{ _current };
    _depth++;
    for (auto &item : _current_list->items) {
        _current->ass[c] = &item;
        for (auto &st : ctx->stmt())
            st->accept(this);
        _current->zrule = rule_t{};
        _current->rules.clear();
        _current->ideps.clear();
    }
    _depth--;

    _current_list = nullptr;
    _lists.erase(c);
    return {};
}

antlrcpp::Any manager::visitListStmt(TParser::ListStmtContext *ctx) {
    auto c = as_id(ctx->ID());
    _current_list = &_lists[c];
    _current_list->name = c;
    _depth++;
    visitChildren(ctx);
    _depth--;
    _current_list = nullptr;
    return {};
}

template <typename Iter, typename Func>
Iter remove_duplicates(Iter begin, Iter end, Func &&eq) {
    auto it = begin;
    Iter next;
    for (; it != end; it = next) {
        next = std::next(it);
        if (next == end)
            break;
        end = std::remove_if(next, end, [&]<typename T>(T &&v) {
            return eq(*it, std::forward<T>(v));
        });
    }
    return end;
}

antlrcpp::Any manager::visitListModifyStmt(TParser::ListModifyStmtContext *ctx) {
    bool desc = ctx->KDesc();
    auto cmp = [desc](const list_item_t &l, const list_item_t &r) {
        if (desc)
            return l.name > r.name;
        return l.name < r.name;
    };
    auto eq = [](const list_item_t &l, const list_item_t &r) {
        return l.name == r.name;
    };
    auto &it = _current_list->items;
    if (ctx->KSort()) {
        std::stable_sort(it.begin(), it.end(), cmp);
        if (ctx->KUnique())
            it.erase(std::unique(it.begin(), it.end(), eq), it.end());
    } else if (ctx->KUnique()) {
        it.erase(remove_duplicates(it.begin(), it.end(), eq), it.end());
    }
    return {};
}

antlrcpp::Any manager::visitIncludeStmt(TParser::IncludeStmtContext *ctx) {
    auto s0 = ctx->Path()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    auto c = as_id(ctx->ID());
    _current_list = &_lists[c];
    if (_debug) {
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Reading list " << c
                << "from " << s0 << '\n';
    }
    _depth++;

    auto [s, flag] = expand(s0);
    if (flag) throw std::runtime_error{ "Glob not allowed in " + s0 };

    {
        std::ifstream fin{ s };
        list_item_t item;
        auto empty = true;
        while (!fin.eof()) {
            S line;
            std::getline(fin, line);
            if (!fin.good()) break;
            line = expand_env(line);
            if (!line.ends_with(" \\")) {
                if (empty) {
                    _current_list->items.emplace_back(list_item_t{ line });
                    continue;
                }
                item.args.emplace_back(line);
                _current_list->items.emplace_back(std::move(item));
                item = list_item_t{};
                empty = true;
            } else {
                line = line.substr(0, line.size() - 2);
                if (empty) {
                    item.name = line;
                    empty = false;
                    continue;
                }
                item.args.emplace_back(line);
            }
        }
    }

    _depth--;
    _current_list = nullptr;
    return {};
}

antlrcpp::Any manager::visitListSearchStmt(TParser::ListSearchStmtContext *ctx) {
    auto s0 = ctx->Path()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    list_search(s0);
    return {};
}

antlrcpp::Any manager::visitListEnumStmtItem(TParser::ListEnumStmtItemContext *ctx) {
    list_item_t item;
    auto flag = false;
    for (auto t : ctx->ListItemToken()) {
        auto [st, glob] = expand(t->getText());
        if (glob) throw std::runtime_error{ "Glob not allow in " + t->getText() };
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
            return it.name == item.name;
        }), s.end());
    return {};
}

antlrcpp::Any manager::visitListInlineEnumStmt(TParser::ListInlineEnumStmtContext *ctx) {
    for (auto t : ctx->ListItemToken()) {
        auto [s, glob] = expand(t->getText());
        if (glob) throw std::runtime_error{ "Glob not allow in " + t->getText() };
        _current_list->items.emplace_back(list_item_t{ std::move(s) });
    }
    return {};
}

antlrcpp::Any manager::visitTemplateStmt(TParser::TemplateStmtContext *ctx) {
    if (_current_rule || !_current_artifact.empty() || _current_build)
        throw std::runtime_error{ "Invalid state when entering template" };

    template_t tmp{ ctx->Token()->getText(), as_id(ctx->ID()) };
    auto prev_builds = std::move(_builds);

    _current_template = &tmp;
    _current_artifact = "";
    _current_build = _current->make_build();

    visitChildren(ctx);
    if (!ctx->Exclamation())
        tmp.arts.emplace(std::move(_current_artifact));

    _current_build.reset();
    _current_artifact = "";
    _current_template = nullptr;

    tmp.builds = std::move(_builds);
    _templates[tmp.name] += std::move(tmp);
    _builds = std::move(prev_builds);
    return {};
}

antlrcpp::Any manager::visitExecuteStmt(TParser::ExecuteStmtContext *ctx) {
    auto s0 = ctx->Path()->getText();
    if (!s0.ends_with('\n')) throw std::runtime_error{ "Lexer messed up with \\n" };
    s0.pop_back();

    auto [st, glob] = expand(s0);
    if (glob) throw std::runtime_error{ "Glob not allow in " + s0 };
    if (_debug)
        std::cerr << std::string(_depth * 2, ' ') << "ajnin: Executing external command " << st << '\n';

    auto ret = system(st.c_str());
    if (ret != 0)
        throw std::runtime_error{ "External command " + st + " failed with " + std::to_string(ret) };

    return {};
}
