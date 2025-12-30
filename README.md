# Astralis (language core + FFI v0 spec scaffold)

This repo is a **starter scaffold** for the Astralis programming language:
- a **language core spec** (keywords, syntax shape, semantics targets)
- an **FFI v0 spec** aimed at *day-one “call C”* via **Clang-powered header import**
- a tiny **seed0 interpreter** in C (runs a minimal Astralis subset)

> Scope note: this is intentionally *not* an OS/editor/shell repo. It’s the language + toolchain foundation.

## Quick start (seed0)

Seed0 currently supports the **Core v0** slice (see `docs/core-v0.md` for the locked contract):
- statements: `set`, `lock`, `if/otherwise`, `loop forever`, `repeat i from A to B`, `define`, `return`, `break`, `continue`
- expressions: literals, identifiers, `()`, binary `+`, function calls
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

## FFI importer (`astrac c-import`)

`tools/astrac_c_import.py` is the Clang-backed importer for Astralis FFI v0. It
parses a header, rejects constructs that are unsupported in v0 (e.g. variadic
functions), and emits a ready-to-use binding into `bindings/`.

Example:

```bash
python tools/astrac_c_import.py examples/ffi/simple_math.h --link c -o bindings/simple_math.astr
```

## Repo layout

- `docs/language-core.md` — **Astralis-Lang**: keywords, grammar outline, semantics targets
- `docs/ffi-v0.md` — **Astralis FFI v0**: C ABI, Clang import, types, linking, safety model
- `docs/grammar.ebnf` — an approximate grammar sketch to drive the parser
- `src/seed0/` — C seed implementation (minimal subset; designed to grow)
- `examples/` — sample programs

## Roadmap (high level)

1. Expand seed0 to cover the **Core** (if/otherwise, loops, functions)
2. Add bytecode + VM (optional early stabilization target)
3. Implement `astrac c-import` (Clang tooling) and foreign declarations
4. Stage1 compiler in Astralis; bootstrap to self-hosting

MIT licensed (see `LICENSE`).
