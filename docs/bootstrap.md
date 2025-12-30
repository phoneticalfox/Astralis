# Bootstrapping plan (Stage0 -> Stage1)

## Stage0 (seed)
- Implemented in C (this repo includes a minimal interpreter).
- Expand until it can parse/execute the **Astralis-Core** subset used to write a compiler.

## Stage1 (compiler in Astralis)
- Write `astrac` in Astralis-Core.
- Use Stage0 to run/compile Stage1.

## Stage2 (self-host)
- Use Stage1 to compile itself.
- Verify determinism by compiling twice and comparing outputs / running a test suite.

## Keep Stage0
Even after self-hosting, keep Stage0 around as:
- a portability bootstrap
- a recovery path if Stage1 breaks
