#include "manager.hpp"

#include <filesystem>
#include <iostream>
#include <stack>

using namespace parsing;

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

    _depth++;
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
            std::cerr << it.name << "[";
            auto flag = false;
            for (auto &f : it.args) {
                if (flag) std::cerr << ",";
                flag = true;
                std::cerr << f;
            }
            std::cerr << "]\n";
        }
    }
    _current_list = nullptr;
    return {};
}

antlrcpp::Any manager::visitListSearchStmt(TParser::ListSearchStmtContext *ctx) {
    // TODO: search file
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
