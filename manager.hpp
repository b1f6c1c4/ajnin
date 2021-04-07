#pragma once

#include <deque>
#include <map>
#include <string>
#include "TParser.h"
#include "TParserBaseVisitor.h"

namespace parsing {
    using S = std::string;
    using SS = std::deque<S>;
    using C = char;
    using CS = std::set<char>;
    template <typename T>
    using MS = std::map<S, T>;
    template <typename T>
    using MC = std::map<C, T>;

    struct rule_t {
        SS deps;
    };

    struct list_item_t {
        S name;
        SS args;
    };

    struct list_t {
        C name;
        std::deque<list_item_t> items;
        CS deps;
    };

    class manager : public TParserBaseVisitor {
        struct ctx_t {
            ctx_t *prev;
            MC<S> ass;
            S operator[](const C &s);
        };

        MS<rule_t> _rules;
        MC<list_t> _lists;

        ctx_t *_current{};
        list_t *_current_list{};

        const bool _debug{};
        const size_t _debug_limit{};
        size_t _depth{};

    public:
        explicit manager(bool debug = false, size_t limit = 15) : _debug{ debug }, _debug_limit{ limit } { }

        antlrcpp::Any visitGroupStmt(TParser::GroupStmtContext *ctx) override;

        antlrcpp::Any visitListStmt(TParser::ListStmtContext *ctx) override;

        antlrcpp::Any visitListSearchStmt(TParser::ListSearchStmtContext *ctx) override;

        antlrcpp::Any visitListEnumStmtItem(TParser::ListEnumStmtItemContext *ctx) override;

        antlrcpp::Any visitListInlineEnumStmt(TParser::ListInlineEnumStmtContext *ctx) override;

        antlrcpp::Any visitPipeStmt(TParser::PipeStmtContext *ctx) override;

        antlrcpp::Any visitRuleStmt(TParser::RuleStmtContext *ctx) override;
    };
}
