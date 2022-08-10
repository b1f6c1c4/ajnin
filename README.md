# [ˈajn in]: A Beautiful [Ninja](https://github.com/ninja-build/ninja) generator

[![AUR](https://img.shields.io/aur/version/ajnin.svg?style=flat-square)](https://aur.archlinux.org/packages/ajnin)
[![AUR](https://img.shields.io/aur/last-modified/ajnin.svg?style=flat-square)](https://aur.archlinux.org/packages/ajnin)
[![GitHub last commit](https://img.shields.io/github/last-commit/b1f6c1c4/ajnin.svg?style=flat-square)](https://github.com/b1f6c1c4/ajnin)
[![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/b1f6c1c4/ajnin.svg?style=flat-square)](https://github.com/b1f6c1c4/ajnin)
[![license](https://img.shields.io/github/license/b1f6c1c4/ajnin.svg?style=flat-square)](https://github.com/b1f6c1c4/ajnin/blob/master/LICENSE)

[![Appveyor Build](https://img.shields.io/appveyor/build/b1f6c1c4/ajnin?style=flat-square)](https://ci.appveyor.com/project/b1f6c1c4/ajnin)
[![Coveralls](https://img.shields.io/coveralls/github/b1f6c1c4/ajnin.svg?style=flat-square)](https://coveralls.io/github/b1f6c1c4/ajnin)

[[ˈnɪ̈̃nʷd͡ʒʷ ə]](https://github.com/ninja-build/ninja) focuses on **speed**,
[['ajn in]](https://github.com/b1f6c1c4/ajnin) focuses on **beauty**.

![Demo](demo.gif)


Code `build.ajnin` **beautifully**:

```
rule cc |= (/usr/bin/cc)
rule cc &cflags+=' -O3'

(all) << {

    list c := src/$$.c
    foreach c {
        (src/$c.c) --cc-- (build/$c.o) --ld-- (build/a.out)
    }

}
```

Run: `ajnin -o build.ninja build.ajnin` to bridge **beauty** and **speed**:

```ninja
build all: phony build/a.out
build build/a.out: ld build/a.o build/b.o
build build/a.o: cc src/a.c | /usr/bin/cc
    cflags =  -O3
build build/b.o: cc src/b.c | /usr/bin/cc
    cflags =  -O3
```

And then run `ninja` **speedy**.

## Install

- Arch Linux: It's on [AUR](https://aur.archlinux.org/packages/ajnin/)
- Linux but not Arch Linux: [git-get](https://github.com/b1f6c1c4/git-get) this repo and use CMake.
    You should have already learned how to use CMake and Ninja.
- Other unlisted operating system: Not officially supported. Go figure it out yourself.

## Usage

`ajnin`: Convert `*.ajnin` into `*.ninja`
```
Usage: ajnin  [-h|--help] [-q|--quiet] [-C <chdir>] [-d|--debug] [-o <output>]
              [-s|--slice <regex>]... [-S|--solo <regex>]... [--bare]
              [<input>]
Note: -s and -S implies --bare, which cannot be override
```

`an`: Convert `*.ajnin` into `*.ninja` and execute
```
Usage: an     [-h|--help] [-q|--quiet] [-C <chdir>] [-o <build.ninja>]
              [-s|--slice <regex>]... [-S|--solo <regex>]... [--bare]
              [-f <build.ajnin>] [<ninja command line arguments>]...
Note: -s and -S implies -o '', but can be override
```

`sanity`: Convert one `*.ajnin` into multiple `*.ninja`s
```
Usage: sanity [-h|--help] [-q|--quiet] [-C <chdir>]
              [-s|--slice <regex>]... [-S|--solo <regex>]...
              [-f <build.ajnin>] [-o <sanity.d>]
              [-j <parallelism>] [<regex>]...
```

## ajnin Language Reference

Checkout `tests/*.ajnin`. You are on your own. Good luck.

## Legal

This project is licensed under **GNU AGPL v3.0** only. (AGPL-3.0-only).

```
Copyright (C) 2021-2022 b1f6c1c4

This file is part of ajnin.

ajnin is free software: you can redistribute it and/or modify it under the
terms of the GNU Affero General Public License as published by the Free
Software Foundation, version 3.

ajnin is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
more details.

You should have received a copy of the GNU Affero General Public License
along with ajnin.  If not, see <https://www.gnu.org/licenses/>.
```
