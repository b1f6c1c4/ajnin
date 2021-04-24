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

void manager::art_to_dep() {
    if (_current_build->deps.empty() && _current_build->ideps.empty())
        _current_build->deps.emplace_back(std::move(_current_artifact));
}

void manager::append_artifact() {
    for (auto ptr = _current; ptr; ptr = ptr->prev)
        if (ptr->app) {
            auto orig_artifact = std::move(_current_artifact);

            auto b = std::make_shared<build_t>(*ptr->app);
            b->deps.emplace_back(orig_artifact);

            auto &pb = _builds[b->art];
            if (!pb) pb = std::make_shared<build_t>();
            *pb += std::move(*b);

            if (ptr->app_also)
                _current_artifact = std::move(orig_artifact);
            else
                _current_artifact = pb->art;
        }
}

antlrcpp::Any manager::visitPipeStmt(TParser::PipeStmtContext *ctx) {
    _current_build = nullptr;
    visitChildren(ctx);
    if (!ctx->templateInst() && !ctx->Exclamation())
        append_artifact();
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
    art_to_dep();

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
    for (auto &[k, v] : _current_build->vars)
        v = expand_art(v);

    auto &pb = _builds[_current_artifact];
    if (!pb) pb = std::make_shared<build_t>();
    *pb += std::move(*_current_build);

    _current_build = std::make_shared<build_t>();
    return {};
}

antlrcpp::Any manager::visitAlsoGroup(TParser::AlsoGroupContext *ctx) {
    auto orig_artifact = _current_artifact;
    visitChildren(ctx);
    if (!ctx->Exclamation()) {
        if (_current_template)
            _current_template->arts.emplace(std::move(_current_artifact));
        else
            append_artifact();
    }
    _current_artifact = std::move(orig_artifact);
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

    _current_rule->vars[as] = std::move(_current_value);
    return {};
}

antlrcpp::Any manager::visitValue(TParser::ValueContext *ctx) {
    if (ctx->ID()) {
        auto id = ctx->ID()->getText();
        if (id.size() != 1) throw std::runtime_error{ "Lexer messed up with ID" };
        if (_current_template && _current_template->par == id[0]) {
            _current_value = '$' + id;
            return {};
        }
        auto a = (*_current)[id[0]];
        if (!a) return {};
        _current_value = a->name;
        return {};
    }
    if (ctx->SubID()) {
        auto id = ctx->SubID()->getText();
        if (id.size() != 2) throw std::runtime_error{ "Lexer messed up with SubID" };
        if (_current_template && _current_template->par == id[0]) {
            _current_value = '$' + id;
            return {};
        }
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

// _current_artifact must be valid.
// _current_build will be cleared.
// _current_artifact will be cleared.
void manager::apply_template(const S &s0, const SS &args) {
    if (_current_template)
        throw std::runtime_error{ "Invalid state when apply_template." };
    if (!_templates.contains(s0))
        throw std::runtime_error{ "Template " + s0 + " not found." };

    auto &tmpl = _templates.at(s0);

    auto spatch = [&](std::string s) {
        for (size_t i{}; i < s.size(); i++) {
            if (s[i] != '$') continue;
            if (i == s.size() - 1) throw std::runtime_error{ "Invalid dep " + s0 };
            if (tmpl.par != s[i + 1]) continue;
            auto [st, sz] = [&]() -> std::pair<S, size_t> {
                if (i != s.size() - 2 && std::isdigit(s[i + 2])) {
                    auto v = s[i + 2] - '0';
                    if (v > args.size())
                        throw std::runtime_error{ "Parameter "s + tmpl.par + " out of range" };
                    return { args[v + 1], 3 };
                }
                return { args[0], 2 };
            }();
            s.replace(i, sz, st);
            i += st.size() - 1;
        }
        return std::move(s);
    };
    auto patch = [&](build_t b) {
        for (auto &s : b.deps)
            s = s.empty() ? s = _current_artifact : spatch(s);
        Ss next;
        for (const auto &s : b.ideps)
            next.emplace(s.empty() ? _current_value : spatch(s));
        b.ideps = std::move(next);
        for (auto &[k, v] : b.vars)
            v = spatch(v);
        b.art = spatch(b.art);
        return b;
    };

    for (auto &[k, v] : tmpl.builds) {
        auto &&pv = patch(*v);
        auto &pb = _builds[pv.art]; // note that art is also patched
        if (!pb) pb = std::make_shared<build_t>();
        *pb += std::move(pv);
    }

    SS na;
    for (auto &a : tmpl.next_args)
        na.emplace_back(spatch(a));

    for (auto &art : tmpl.arts) {
        _current_artifact = spatch(art);
        if (!tmpl.next.empty())
            apply_template(tmpl.next, na);
        else
            append_artifact();
    }

    _current_artifact = {};
    _current_build = nullptr;
}

// When _current_template is nullptr:
//    _current_artifact must be valid.
//    _current_build will be cleared.
//    _current_artifact will be cleared.
// Otherwise:
//    _current_artifact will not change.
//    _current_build will not change.
antlrcpp::Any manager::visitTemplateInst(TParser::TemplateInstContext *ctx) {
    auto s0 = ctx->TemplateName()->getText();
    if (!s0.starts_with('<') || !s0.ends_with('>')) throw std::runtime_error{ "Lexer messed up with <>" };
    s0 = s0.substr(1, s0.length() - 2);

    SS args;
    for (auto v : ctx->value()) {
        v->accept(this);
        args.emplace_back(std::move(_current_value));
    }

    if (!_current_template) {
        if (!ctx->Exclamation())
            apply_template(s0, args);
    } else {
        if (ctx->Exclamation())
            throw std::runtime_error{ "Exclamation mark not allowed on template inst inside template decl." };
        _current_template->next = std::move(s0);
        _current_template->next_args = std::move(args);
    }

    return {};
}