# Bootstrapping plan (Stage0 -> Stage1)

## Stage0 (seed)
- Implemented in C (this repo includes the interpreter + importer prototype).
- Grow coverage to the **Core v0** contract (ifs/loops/define/calls/strings/ints) — most is already parsed and executed.
- Add basic tooling glue: example runner (`tools/run_examples.sh`) and importer (`tools/astrac_c_import.py`).

## Stage1 (compiler in Astralis)
- Write `astrac` in Astralis-Core using Stage0 as the executor.
- Emit either x86_64 (to match FFI v0) or `.astrb` bytecode for the VM path.
- Use the importer output to validate the foreign-function surface.

## Stage2 (self-host)
- Use Stage1 to compile itself.
- Verify determinism by compiling twice and comparing outputs / running a test suite.
- Retire the C interpreter from the critical path but keep it runnable for regression and bootstrap.

## Keep Stage0
Even after self-hosting, keep Stage0 around as:
- a portability bootstrap
- a recovery path if Stage1 breaks
- a debugger-friendly interpreter for quick repros of parser/runtime bugs
