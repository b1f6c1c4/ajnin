/* Copyright (C) 2021-2023 b1f6c1c4
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

#include <boost/regex.hpp>
#include <filesystem>
#include <deque>
#include <string>
#include <memory>

namespace parsing {
    struct filter {
        ~filter() = default;
        virtual int operator()(const std::string &art) const = 0;
    };
    using pfilter = std::shared_ptr<filter>;

    struct cascade_filter : filter {
        cascade_filter(pfilter first, pfilter next) : _first{ std::move(first) }, _next{ std::move(next) } { }
        int operator()(const std::string &art) const override {
            if (auto ret = (*_first)(art))
                return ret;
            return (*_next)(art);
        }

    private:
        pfilter _first, _next;
    };

    struct solo_filter : filter {
        explicit solo_filter(const std::deque<std::string> &re) : _re{} {
            for (auto &r : re)
                _re.emplace_back(r);
        }
        int operator()(const std::string &art) const override {
            if (_re.empty()) return 0;
            if (std::any_of(_re.begin(), _re.end(), [&art](const boost::regex &re) {
                return boost::regex_match(art, re);
            }))
                return +1;
            return -1;
        }
    private:
        std::deque<boost::regex> _re;
    };

    struct slice_filter : filter {
        explicit slice_filter(const std::deque<std::string> &re) : _re{} {
            for (auto &r : re)
                _re.emplace_back(r);
        }
        int operator()(const std::string &art) const override {
            if (_re.empty()) return 0;
            if (std::any_of(_re.begin(), _re.end(), [&art](const boost::regex &re) {
                return boost::regex_match(art, re);
            }) && std::filesystem::exists(art))
                return -1;
            return +1;
        }
    private:
        std::deque<boost::regex> _re;
    };
}
