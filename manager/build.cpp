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
#include <stack>
#include "TLexer.h"

using namespace parsing;
using namespace std::string_literals;

antlrcpp::Any manager::visitPipeStmt(TParser::PipeStmtContext *ctx) {
    _current_build = nullptr;
    ctx->pipe()->accept(this);
    for (auto ptr = _current; ptr; ptr = ptr->prev)
        if (ptr->app) {
            auto orig_artifact = _current_artifact;

            auto b = std::make_shared<build_t>(*ptr->app);
            b->deps.emplace_back(std::move(_current_artifact));

            auto &pb = _builds[b->art];
            if (!pb) pb = std::make_shared<build_t>();
            *pb += std::move(*b);

            if (ptr->app_also)
                _current_artifact = orig_artifact;
        }
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

antlrcpp::Any manager::visitAlsoGroup(TParser::AlsoGroupContext *ctx) {
    auto orig_artifact = _current_artifact;
    visitChildren(ctx);
    _current_artifact = orig_artifact;
    return {};
}

antlrcpp::Any manager::visitAssignment(TParser::AssignmentContext *ctx) {
    auto as = ctx->Assign()->getText();
    if (!as.starts_with('&') || !as.ends_with('=')) throw std::runtime_error{ "Lexer messed up with &=" };
    auto append = as[as.size() - 2] == '+';
    as = as.substr(1, as.length() - (append ? 3 : 2));

    if (!ctx->value()) {
        _current_rule->vars.erase(as);
        return {};
    }

    ctx->value()->accept(this);
    auto &rule = _current_rule->name;
    if (append)
        _current_value = (*_current)[rule].vars[as] + _current_value;

    _current_rule->vars[as] = _current_value;
    return {};
}

antlrcpp::Any manager::visitValue(TParser::ValueContext *ctx) {
    if (ctx->ID()) {
        auto id = ctx->ID()->getText();
        if (id.size() != 1) throw std::runtime_error{ "Lexer messed up with ID" };
        auto a = (*_current)[id[0]];
        if (!a) return {};
        _current_value = a->name;
        return {};
    }
    if (ctx->SubID()) {
        auto id = ctx->SubID()->getText();
        if (id.size() != 2) throw std::runtime_error{ "Lexer messed up with SubID" };
        auto a = (*_current)[id[0]];
        if (!a) return {};
        auto v = id[1] - '0';
        if (v >= a->args.size())
            return {};
        _current_value = a->args[v];
        return {};
    }
    if (ctx->SingleString()) {
        auto s0 = ctx->SingleString()->getText();
        if (!s0.starts_with('\'') || !s0.ends_with('\'')) throw std::runtime_error{ "Lexer messed up with ''" };
        s0 = s0.substr(1, s0.size() - 2);
        s0 = expand_quote(s0, '\'');
        s0 = expand_env(s0);
        _current_value = std::move(s0);
        return {};
    }
    if (ctx->DoubleString()) {
        auto s0 = ctx->DoubleString()->getText();
        if (!s0.starts_with('"') || !s0.ends_with('"')) throw std::runtime_error{ "Lexer messed up with \"\"" };
        s0 = s0.substr(1, s0.size() - 2);
        s0 = expand_quote(s0, '"');
        auto [s, glob] = expand(s0);
        if (glob) throw std::runtime_error{ "Glob not allow in " + s0 };
        _current_value = std::move(s);
        return {};
    }
    throw std::runtime_error{ "Invalid value" };
}

antlrcpp::Any manager::visitTemplateInst(TParser::TemplateInstContext *ctx) {
    return TParserBaseVisitor::visitTemplateInst(ctx);
}
