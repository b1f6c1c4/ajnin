# [ˈajn in]: A Beautiful [Ninja](https://github.com/ninja-build/ninja) generator

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

[git-get](https://github.com/b1f6c1c4/git-get) this repo and use CMake.
You should have already learned how to use CMake and Ninja.

## Command line

```
Usage: ajnin [-h|--help] [-d|--debug] [-o <output>] [<file>]
```

## Legal

This project is licensed under **GNU AGPL v3.0** only. (AGPL-3.0-only).

