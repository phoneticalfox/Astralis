# Astralis (language core + FFI v0 reference)

This repo hosts the **concrete reference** for the Astralis programming language:
- a **language core spec** that the seed toolchain actually implements
- a **normative FFI v0 spec** for day-one “call C” via **Clang-powered header import**
- a working **seed0 interpreter** in C that runs the Core v0 slice

> Scope note: this is intentionally *not* an OS/editor/shell repo. It’s the language + toolchain foundation.

## Quick start (seed0)

Seed0 currently supports the **Core v0** slice (see `docs/core-v0.md` for the locked contract):
- statements: `set`, `lock`, `if/otherwise`, `loop forever`, `repeat i from A to B`, `define`, `return`, `break`, `continue`
- expressions: literals (strings/ints), identifiers, grouping `()`, binary `+`, function calls
- I/O: `show` / `say`, `warn`, `ask("prompt")`

Build:
```bash
cd src/seed0
make
```

Run:
```bash
./astralis ../../examples/hello.astr
```

Regression suite (examples):
```bash
# from repo root, after building seed0
tools/run_examples.sh
```

## FFI importer prototype (real)

`tools/astrac_c_import.py` is a functional importer: it shells out to Clang's
JSON AST to generate foreign module stubs from a header file and writes the
result into `bindings/`.

Example:

```bash
python tools/astrac_c_import.py examples/ffi/simple_math.h --link c -o bindings/simple_math.astr
```

## Repo layout

- `docs/language-core.md` — **Astralis-Lang**: keywords, grammar outline, semantics targets
- `docs/ffi-v0.md` — **Astralis FFI v0**: C ABI, Clang import, types, linking, safety model
- `docs/grammar.ebnf` — the current parser grammar for the seed0 interpreter
- `src/seed0/` — C seed implementation (minimal subset; designed to grow)
- `examples/` — sample programs

## Roadmap (high level)

1. Expand seed0 to cover the **Core** (conditionals, loops, functions — partially done)
2. Add bytecode + VM (optional early stabilization target)
3. Harden `astrac c-import` (Clang tooling) and foreign declarations against the normative FFI spec
4. Stage1 compiler in Astralis; bootstrap to self-hosting

MIT licensed (see `LICENSE`).
