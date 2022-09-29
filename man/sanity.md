% SANITY(1) ajnin v0.5 | ajnin v0.5 manual
% b1f6c1c4 <b1f6c1c4@gmail.com>
% August 2021

# NAME

**sanity** - A Beautiful, Distributive Ninja generator

# SYNOPSIS

**sanity** `[options]` `<endpoint-regex>...`

# DESCRIPTION

**sanity** takes **ajnin**'s own DSL from **build.ajnin** (adjustable by **-f**)
and generates a number of **build.ninja** files
in **sanity.d/** (adjustable by **-o**).

The choice of `<endpoint-regex>` is critical.
Only the specified targets will be distributively built.
If you specify a capture group, then the captured text will be used to
determine which **build.ninja** file the target belong to.
Note that any target that is shared by more than one
**build.ninja** is placed in the common **build.ninja** and
is thus not built distributively.

# OPTION

**-h**, `--help`
: Show brief usage information.

**-q**, `--quiet`
: Display less information during execution.

**-d**, `--debug`
: Display more information during execution.

**-C** `<chdir>`
: Run **sanity** as if from that directory.

**-j** `<parallelism>`
: **Required**. Indicates how many distributive **build.ninja** files to generate.

**-o** `<output-dir>`
: Write **ninja(1)** configurations into directory *`<output-dir>`*.
Defaults to **sanity.d/**.
*`<output-dir>/build.ninja`* will contain the common part, and
*`<output-dir>/<I>/build.ninja`* will contain the distributed part.

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

**-f** `<input>`
: File containing **ajnin DSL** to be read from.
Defaults to **build.ajnin**.
If not specified, stdin will be used and **`--bare`** is assumed.

# EXAMPLE

Suppose you want to distribute a gigantic C/C++ build into 8 builds:

```bash
sanity -j8 'build/.*\.o'
```

You may wish to run the distributive jobs on slurm:

```bash
#!/bin/sh
#SBATCH --job-name=...
#SBATCH --ntasks=8           # spawn 8 ninja
#SBATCH --cpus-per-task=2    # each ninja spawns 2 tasks
#SBATCH --mem-per-cpu=...
#SBATCH --time=...
#SBATCH --output=...

ninja -f "sanity.d/build.ninja"
srun sh -c "ninja -f sanity.d/\$SLURM_PROCID/build.ninja -j2"
awk 'FNR>1 { print; }' sanity.d/*/.ninja_log >> .ninja_log
```

# SEE ALSO

**ajnin(1)**, **ninja(1)**
