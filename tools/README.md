# Tools

## Regression runner

`run_examples.sh` executes every `.astr` program in `examples/` (skipping files with a matching `.skip` flag), feeds optional `.in` input files, and diffs outputs against the expected `.out` snapshots. Run it from the repo root after building `src/seed0/astralis`.

## `astrac c-import` (planned)

See `docs/ffi-v0.md` for the contract.

Implementation options:
- call out to `clang` / `libclang` to parse headers and emit `bindings/*.astr`
- cache generated bindings keyed by:
  - header path + content hash
  - target triple
  - include paths + defines
