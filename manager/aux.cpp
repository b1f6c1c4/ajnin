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

#include <cctype>
#include <cstring>
#include <filesystem>
#include <iostream>
#include "TLexer.h"

using namespace parsing;
using namespace std::string_literals;

rule_t &rule_t::operator+=(const rule_t &o) {
    if (!name.empty() && !o.name.empty() && name != o.name)
        throw std::runtime_error{ "Cannot add rules under different names" };
    if (!o.name.empty())
        name = o.name;
    for (auto &[k, v] : o.vars)
        vars[k] = v;
    for (auto &dep : o.ideps)
        ideps.insert(dep);
    for (auto &dep : o.iideps)
        iideps.insert(dep);
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
    for (auto &dep : iideps)
        pb->iideps.insert(dep);
    return pb;
}

std::filesystem::path manager::ctx_t::get_cwd() const {
    if (cwd.has_value()) return cwd.value();
    if (!prev) throw std::runtime_error{ "Invalid ctx: No cwd" };
    return prev->get_cwd();
}

manager::ctx_t manager::ctx_t::save() const {
    if (!prev) return *this;
    auto ctx = *this;
    {
        auto &&p = prev->save();
        ctx.ass.merge(p.ass);
        ctx.zrule += p.zrule;
        for (auto &[k, v] : p.rules)
            ctx.rules[k] += v;
        ctx.ideps.merge(p.ideps);
        ctx.iideps.merge(p.iideps);
    }
    return ctx;
}

build_t &build_t::operator+=(build_t &&o) {
    if (art.empty())
        return *this = std::move(o);

    if (rule != o.rule)
        throw std::runtime_error{ "Conflict rule for " + art };
    if (!std::equal(vars.begin(), vars.end(), o.vars.begin()))
        throw std::runtime_error{ "Conflict var for " + art };

    if (!o.deps.empty())
        dirty = true;
    for (auto &dep : o.deps)
        deps.emplace_back(std::move(dep));
    o.deps.clear();
    return *this;
}

bool build_t::dedup() {
    if (!dirty) return false;
    SS next;
    Ss seen;
    for (auto &dep : deps)
        if (seen.insert(dep).second)
            next.emplace_back(std::move(dep));
    auto found = deps.size() != next.size();
    deps = std::move(next);
    dirty = false;
    return found;
}

manager::template_t &manager::template_t::operator+=(manager::template_t &&o) {
    if (name.empty())
        return *this = std::move(o);

    if (name != o.name)
        throw std::runtime_error{ "Conflict name" };
    if (par != o.par)
        throw std::runtime_error{ "Conflict par for " + name };

    arts.merge(o.arts);

    for (auto &[k, v] : o.builds) {
        auto &pb = builds[k];
        if (!pb) pb = std::make_shared<build_t>();
        *pb += std::move(*v);
    }

    for (auto &n : o.nexts)
        nexts.emplace_back(std::move(n));
    return *this;
}

bool manager::template_t::dedup() {
    auto found = false;
    for (auto &[k, v] : builds)
        found |= v->dedup();
    return found;
}

S manager::expand_dollar(S s) {
    for (auto &c : s)
        if (c == '\e')
            c = '$';
    return s;
}

S manager::expand_quote(S s, char c) {
    for (size_t i{}; i < s.size(); i++) {
        if (s[i] != '$') continue;
        if (i == s.size() - 1) continue;
        if (s[i + 1] == c) {
            s.replace(i, 1, "");
            i++;
        }
    }
    return s;
}

C manager::as_id(antlr4::tree::TerminalNode *s) {
    if (!s) return '\0';
    return s->getText()[0];
}

S manager::expand_env(const S &s0) const {
    auto s = s0;
    for (size_t i{}; i < s.size(); i++) {
        if (s[i] != '$') continue;
        if (i == s.size() - 1) throw std::runtime_error{ "Invalid string " + s0 };
        switch (s[i + 1]) {
            case '/': {
                auto p = _current->get_cwd().string();
                s.replace(i, 1, p);
                i += p.size() - 1;
                continue;
            }
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
        i += std::strlen(st) - 1;
    }
    return s;
}

S manager::expand_art(const S &s0) const {
    auto s = s0;
    for (size_t i{}; i < s.size(); i++) {
        if (s[i] != '$') continue;
        if (i == s.size() - 1) throw std::runtime_error{ "Invalid string " + s0 };
        if (s[i + 1] == '@') {
            s.replace(i, 2, _current_artifact);
            i += _current_artifact.size() - 1;
            continue;
        }
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
        if (_current_rule && s[i + 1] == '@') {
            i++;
            continue;
        }
        if (_current_template && _current_template->par == s[i + 1]) {
            if (i != s.size() - 2 && std::isdigit(s[i + 2]))
                i++;
            i++;
            continue;
        }
        auto [st, sz] = [&]() -> std::pair<S, size_t> {
            auto a = (*_current)[s[i + 1]];
            if (!a) throw std::runtime_error{ "List "s + s[i + 1] + " not enumerated yet"};
            if (i != s.size() - 2 && std::isdigit(s[i + 2])) {
                auto v = s[i + 2] - '0';
                if (v >= a->args.size())
                    return { ""s, 3 };
                return { a->args[v], 3 };
            }
            return { a->name, 2 };
        }();
        s.replace(i, sz, st);
        i += st.size() - 1;
    }
    return { s, flag };
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
static void glob_search(P directory, PC start, const PC &finish, const S &filename, std::function<bool(const S &)> cb) {
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

void manager::list_search(const S &s0) {
    auto [s, flag] = expand(s0);
    if (!flag) throw std::runtime_error{ "No glob in " + s0 };
    auto id = s.find("$$");

    std::filesystem::path p{ s };
    auto ins = [&](const P &pa) {
        auto p = pa.string();
        auto str = s;
        str.replace(id, 2, p);
        _current_list->items.emplace_back(list_item_t{
                std::move(p),
                { std::move(str) }
        });
        return false;
    };
    if (p.is_absolute()) {
        auto rp = p.parent_path().relative_path();
        glob_search(p.root_path(), std::begin(rp), std::end(rp), p.filename().string(), ins);
    } else {
        auto pa = p.parent_path();
        glob_search(std::filesystem::current_path(), std::begin(pa), std::end(pa), p.filename().string(), ins);
    }
}
