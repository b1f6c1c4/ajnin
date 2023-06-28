% AJNIN(1) ajnin v0.6 | ajnin v0.6 manual
% b1f6c1c4 <b1f6c1c4@gmail.com>
% August 2021

# NAME

**ajnin** - A Beautiful Ninja generator, the core part.

# SYNOPSIS

**ajnin** `[-o <output>]` `[options]` `[<input>]`

# DESCRIPTION

**ajnin** generates **ninja(1)** configuration file from **ajnin**'s own DSL.
See **EXAMPLES** section for how to write the DSL.
If you wish to run **ninja(1)** immediately after running **ajnin**,
please refer to **an(1)**, which stands for **ajnin+ninja**.

# OPTION

**-h**, `--help`
: Show brief usage information.

**-q**, `--quiet`
: Display less information during execution.

**-d**, `--debug`
: Display more information during execution.

**-C** `<chdir>`
: Run **ajnin** as if from that directory.

**-o** `<output>`
: Write **ninja(1)** configuration into file *`<output>`*.
If `<output>` is up-to-date
(based on the previously-written **mtime** information in *`<output>`*),
no rewriting happens.
See also **`--bare`**.
If not specified, stdout will be used and **`--bare`** is assumed.

`--bare`
: Do *not* write *`<input>`*'s **mtime** into *`<output>`*.
This will make sure that any following invocation of
*`ajnin -o <output>`* will always overwrite.

**-s**, `--slice` `<regex>`
: Skip some targets when generating configuration file base on the two conditions:
(*both* of the two conditions must be satisfied for **ajnin** to skip the target)
**1)** the target name matches *`<regex>`*;
**2)** the target exists on disk.
**`--slice`** implies **`--bare`**.

**-S**, `--solo` `<regex>`
: Include only those targets whose name matches *`<regex>`*
when generating configuration file.
**`--solo`** implies **`--bare`**.

`<input>`
: File containing **ajnin DSL** to be read from.
If not specified, stdin will be used and **`--bare`** is assumed.

# EXAMPLE

Please refer to <https://github.com/b1f6c1c4/ajnin/tree/master/tests> for some DSL snippets.

# SEE ALSO

**an(1)**, **sanity(1)**, **ninja(1)**
