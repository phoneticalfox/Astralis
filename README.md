# Astralis (language core + FFI v0 spec scaffold)

This repo is a **starter scaffold** for the Astralis programming language:
- a **language core spec** (keywords, syntax shape, semantics targets)
- an **FFI v0 spec** aimed at *day-one “call C”* via **Clang-powered header import**
- a tiny **seed0 interpreter** in C (runs a minimal Astralis subset)

> Scope note: this is intentionally *not* an OS/editor/shell repo. It’s the language + toolchain foundation.

## Quick start (seed0)

Seed0 currently supports:
- `show "string"` / `say "string"`
- `warn "string"`
- `set name to "string"` / `set name to 123`
- `lock name to ...` (same as `set`, but immutable)
- `ask("prompt")` (reads a line)

Build:
```bash
cd src/seed0
make
```

Run:
```bash
./astralis ../../examples/hello.astr
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
