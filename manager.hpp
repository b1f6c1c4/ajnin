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

#pragma once

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include "TParser.h"
#include "TParserBaseVisitor.h"

namespace parsing {
    using S = std::string;
    using SS = std::deque<S>;
    using Ss = std::set<S>;
    using C = char;
    using CS = std::set<char>;
    template <typename T>
    using MS = std::map<S, T>;
    template <typename T>
    using MC = std::map<C, T>;

    struct rule_t {
        S name;
        MS<S> vars;
        Ss ideps;

        rule_t &operator+=(const rule_t &o);
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

    struct build_t {
        S art;
        S rule;
        SS deps;
        Ss ideps;
        MS<S> vars;

        build_t &operator+=(build_t &&o);
    };
    using pbuild_t = std::shared_ptr<build_t>;

    class manager : public TParserBaseVisitor {
        struct ctx_t {
            ctx_t *prev;
            MC<list_item_t *> ass;
            rule_t zrule;
            MS<rule_t> rules;
            Ss ideps;
            pbuild_t app;
            bool app_also;

            [[nodiscard]] list_item_t *operator[](const C &s) const;
            [[nodiscard]] rule_t operator[](const S &s) const;
            [[nodiscard]] pbuild_t make_build() const;
        };

        MC<list_t> _lists;
        MS<pbuild_t> _builds;

        ctx_t *_current{};
        list_t *_current_list{};
        pbuild_t _current_build{};
        rule_t *_current_rule{};
        S _current_artifact{};

        SS _prolog, _epilog;

        mutable std::set<S> _env_notif;

        const bool _debug{}, _quiet{};
        const size_t _debug_limit{};
        size_t _depth{};

        [[nodiscard]] S expand_env(const S &s0) const;
        [[nodiscard]] std::pair<S, bool> expand(const S &s0) const;
        void list_search(const S &s0);

    public:
        explicit manager(bool debug = false, bool quiet = false, size_t limit = 15);

        antlrcpp::Any visitMain(TParser::MainContext *ctx) override;

        antlrcpp::Any visitDebugStmt(TParser::DebugStmtContext *ctx) override;

        antlrcpp::Any visitClearStmt(TParser::ClearStmtContext *ctx) override;

        antlrcpp::Any visitIfStmt(TParser::IfStmtContext *ctx) override;

        antlrcpp::Any visitRuleStmt(TParser::RuleStmtContext *ctx) override;

        antlrcpp::Any visitGroupStmt(TParser::GroupStmtContext *ctx) override;

        antlrcpp::Any visitListGroupStmt(TParser::ListGroupStmtContext *ctx) override;

        antlrcpp::Any visitListStmt(TParser::ListStmtContext *ctx) override;

        antlrcpp::Any visitListModifyStmt(TParser::ListModifyStmtContext *ctx) override;

        antlrcpp::Any visitIncludeStmt(TParser::IncludeStmtContext *ctx) override;

        antlrcpp::Any visitListSearchStmt(TParser::ListSearchStmtContext *ctx) override;

        antlrcpp::Any visitListEnumStmtItem(TParser::ListEnumStmtItemContext *ctx) override;

        antlrcpp::Any visitListInlineEnumStmt(TParser::ListInlineEnumStmtContext *ctx) override;

        antlrcpp::Any visitPipeStmt(TParser::PipeStmtContext *ctx) override;

        antlrcpp::Any visitPipeGroup(TParser::PipeGroupContext *ctx) override;

        antlrcpp::Any visitArtifact(TParser::ArtifactContext *ctx) override;

        antlrcpp::Any visitPipe(TParser::PipeContext *ctx) override;

        antlrcpp::Any visitStage(TParser::StageContext *ctx) override;

        antlrcpp::Any visitOperation(TParser::OperationContext *ctx) override;

        antlrcpp::Any visitAlsoGroup(TParser::AlsoGroupContext *ctx) override;

        antlrcpp::Any visitAssignment(TParser::AssignmentContext *ctx) override;

        antlrcpp::Any visitProlog(TParser::PrologContext *ctx) override;

        antlrcpp::Any visitEpilog(TParser::EpilogContext *ctx) override;

        friend std::ostream &operator<<(std::ostream &os, const manager &mgr);
    };

    std::ostream &operator<<(std::ostream &os, const manager &mgr);
}
