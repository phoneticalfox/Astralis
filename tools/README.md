# Tools

## `astrac c-import` (planned)

See `docs/ffi-v0.md` for the contract.

Implementation options:
- call out to `clang` / `libclang` to parse headers and emit `bindings/*.astr`
- cache generated bindings keyed by:
  - header path + content hash
  - target triple
  - include paths + defines
