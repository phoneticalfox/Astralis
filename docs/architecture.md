# Compiler/Runtime Architecture (seed0 reality check)

## What exists today
- **Lexer (`src/seed0/lexer.*`)** — whitespace-aware, produces indentation via `col` to drive block parsing.
- **Parser (`src/seed0/parser.*`)** — builds a concrete AST for Core v0 statements (ifs/loops/repeat/define/call/etc.).
- **Interpreter (`src/seed0/interp.*`, `runtime.*`, `value.*`)** — eager, tree-walk execution with an `Env` stack for functions and locals.

The AST and runtime types are intentionally simple: values are tagged unions (null, bool, int, string, list stub), and functions capture a `Block` plus parameters.

## Near-term growth plan
- **Desugar pass**: normalize connectors (`->`, `as`, `:`) and inline bodies before interpretation/codegen.
- **Type tightening**: add runtime errors for unsupported ops (e.g., non-int `+`) and grow the value model (lists/maps).
- **Bytecode VM**: introduce a compiler lowering AST -> bytecode and a small VM to execute `.astrb` artifacts.

## Long-term pipeline sketch
1. **Front end**: lexer -> parser -> validated AST.
2. **Lowering**: optional desugar + semantic checks (names, arity, purity flags when added).
3. **Backends**:
   - **Interpreter** (kept for debugging and bootstrap)
   - **Native codegen**: x86_64 baseline (aligned with FFI v0 ABI target)
   - **Bytecode**: VM path for fast iteration and portability

## Runtime boundaries
- FFI calls sit behind explicit `unsafe:` surfaces per `docs/ffi-v0.md`.
- Errors are surfaced as runtime strings today; structured error values are planned once the value model is richer.
