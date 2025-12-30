# Tools

## Regression runner

`run_examples.sh` executes every `.astr` program in `examples/` (skipping files with a matching `.skip` flag), feeds optional `.in` input files, and diffs outputs against the expected `.out` snapshots. Run it from the repo root after building `src/seed0/astralis`.

## `astrac c-import`

`tools/astrac_c_import.py` is the v0 implementation. It shells out to Clang,
parses the JSON AST, and emits `bindings/*.astr` files with `foreign type`,
`foreign define`, and `foreign declare` entries. Unsupported constructs such as
variadic functions raise errors instead of emitting unusable code.

Usage example:

```
python tools/astrac_c_import.py examples/ffi/simple_math.h --link c -o bindings/simple_math.astr
```
