# Core v0 contract

Core v0 locks the smallest executable slice of Astralis that seed0 will implement next. It is intentionally narrow so that CI can treat it as a non-drifting contract.

## Syntax and constructs

- **Statements**: `set`, `lock`, `if ... otherwise`, `loop forever`, `repeat <var> from <A> to <B>`, `define name(params):`, `return`, `break`, `continue`.
- **Expressions**: literals (`string`, `number`), identifiers, grouping `()`, binary `+`, and function calls.
- **I/O**: `show`, `say`, `warn`, `ask()` built-in.
- **Blocks**: indentation-based; the canonical connector for block bodies is `:` with `->`/`as` kept as inline sugar.

## Semantics snapshot

- **Truthiness**: `null`, `false`, `0`, empty strings, and errors are falsey; everything else is truthy.
- **Errors**: seed0 uses error-as-value; execution halts the current statement when an error bubbles up.
- **Functions**: lexical scoping with captured environment; arity is fixed; `return` yields `null` when omitted.
- **Loops**: `loop forever` honors `break`/`continue`; `repeat i from A to B` counts upward and binds/updates the loop variable in the current scope.
- **Immutability**: `lock` creates bindings that cannot be reassigned.

## Regression surface

Examples under `examples/` (except those with `.skip`) form the Core v0 regression suite. `tools/run_examples.sh` runs them through the seed0 interpreter and diffs outputs against the expected `.out` files.
