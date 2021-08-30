% AN(1) ajnin v0.2 | ajnin v0.2 manual
% b1f6c1c4 <b1f6c1c4@gmail.com>
% August 2021

# NAME

**an** - A Beautiful Ninja generator + A small build system with a focus on speed

# SYNOPSIS

**an** `[options]` `[<ninja command line argument>]...`

# DESCRIPTION

**an** takes **ajnin**'s own DSL from **build.ajnin** (adjustable by **-f**),
generates **build.ninja** (adjustable by **-o**),
and run **ninja(1)** with arguments passed thru.

# OPTION

**-h**, **--help**
: Show brief usage information.

**-q**, **--quiet**
: Display less information during execution.

**-d**, **--debug**
: Display more information during execution.

**-C** `<chdir>`
: Run **ajnin** as if from that directory.

**-o** `<output>`
: Write **ninja(1)** configuration into file `<output>`.
Defaults to **build.ninja**.
If `<output>` is up-to-date
(based on the previously-written **mtime** information in `<output>`),
no rewriting happens.
See also **--bare**.
If specified an empty string,
configuration will be directly fed to **ninja** process without storing it on disk.

**--bare**
: Do *not* write `<input>`'s **mtime** into `<output>`.
This will make sure that any following invocation of
**ajnin** `-o` `<output>` will *always* overwrite.

**-s**, **--slice** `<regex>`
: Skip some targets when generating configuration file base on the two conditions:
(*both* of the two conditions must be satisfied for **ajnin** to skip the target)
**1)** the target name matches `<regex>`;
**2)** the target exists on disk.
**--slice** implies **--bare**.

**-S**, **--solo** `<regex>`
: Include only those targets whose name matches `<regex>`
when generating configuration file.
**--solo** implies **--bare**.

**-f** `<input>`
: File containing **ajnin DSL** to be read from.
Defaults to **build.ajnin**.
If not specified, stdin will be used and **--bare** is assumed.

# EXAMPLE

Please refer to <https://github.com/b1f6c1c4/ajnin/tree/master/tests> for some DSL snippets.

# SEE ALSO

**ajnin(1)**, **ninja(1)**
