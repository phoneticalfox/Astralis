# Core decisions (tracked)

1) Connector token: `:` vs `->` vs `as`
- Core v0 canonicalizes `:` for block bodies. `->`/`as` stay as inline sugar when the body lives on the same line; keep accepting them for compatibility.

2) Errors: exception vs error-as-value
- Core v0 interpreter keeps error-as-value; statements abort when evaluation returns an error.

3) Blocks:
- Indentation-sensitive parsing remains the model for nested blocks.

4) Truthiness model:
- Falsey values are `null`, `false`, `0`, empty strings, and errors; everything else is truthy.

5) Functions:
- Function bodies use the same `:`/inline sugar rule as other blocks. Returns default to `null` when omitted; arity must match the declared parameter list.


## FFI v0 decisions (tracked)

1) Baseline target triple: `x86_64-linux-gnu` (SysV AMD64).

2) Variadic C functions: **wrapper-required** in v0 (direct varargs calls are unsupported).

3) Foreign calls: require an explicit `unsafe:` boundary.
