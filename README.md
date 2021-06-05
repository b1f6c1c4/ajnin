# ajnin [ˈajn in]: A Beautiful Ninja generator

![Demo](demo.gif)

Our top priority is beauty. Look at the DSL:

`build.ajnin`:

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

Run: `ajnin -o build.ninja build.ajnin`:

```ninja
build all: phony build/a.out
build build/a.out: ld build/a.o build/b.o
build build/a.o: cc src/a.c | /usr/bin/cc
    cflags =  -O3
build build/b.o: cc src/b.c | /usr/bin/cc
    cflags =  -O3
```

## Install

[git-get](https://github.com/b1f6c1c4/git-get) this repo and use CMake.
You should have already learned how to use CMake and Ninja.

## Command line

```
Usage: ajnin [-h|--help] [-d|--debug] [-o <output>] [<file>]
```

## Legal

This project is licensed under **GNU AGPL v3.0** only. (AGPL-3.0-only).

