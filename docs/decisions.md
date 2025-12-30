# Open decisions (tracked)

1) Connector token: `->` vs `as`
- v0: accept both; canonicalize in formatter/linter later.

2) Errors: exception vs error-as-value
- v0 interpreter currently uses error-as-value for I/O and control flow short-circuits on errors.

3) Blocks:
- v0 interpreter uses indentation-sensitive parsing for nested blocks.

4) Truthiness model:
- falsey values are `null`, `false`, `0`, empty strings, and errors; everything else is truthy.

5) Canonical block connector for functions and control flow:
- `:` is treated as the canonical block opener; `->`/`as` are accepted as inline sugar when the body stays on the same line.
