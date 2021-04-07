# ajnin: Yet another Ninja generator

Design choices:

- Only generate `build`s, not `rule`s.
- Input should be in a graphical, terse DSL.
- Must be very fast.

Simple example:

```
# Define list
list p := src/$$

# For-each element
each p {
    list c := src/$p/$$.c
    list s := src/$p/$$.s
}

p*c {
    (src/$p/$c.c) --cc-- (build/$p/$c.o) >>ld-- (build/bin/$p)
}

p {
    s {
        (src/$p/$s.c) --as-- (build/$p/$s.o) >>ld-- (build/bin/$p)
    }
}
```

Syntax:

```
<list-decl-by-file> ::= "list" ListIdentifier ":=" Pattern
<list-decl-by-enum> ::= "list" ListIdentifier "::" "\n" <list-decl-by-enum-item>
<list-decl-by-enum-item> ::= "    " Token [ " " Token ]*
```

Complex example:

```
# Define list
list i ::
    a1 f1 f2 f3
    a2 f1 f2
    a3 f1 f2 f3
list t ::
    caoi

# Define dep
rule verilator |= common.svh

i {
    (instr.sv) --verilator$vltflags=i1-- (build/instr/$i.sv) --synth$t-- 
}
```
