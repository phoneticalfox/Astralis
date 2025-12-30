# Open decisions (tracked)

1) Connector token: `->` vs `as`
- v0: accept both; canonicalize in formatter/linter later.

2) Errors: exception vs error-as-value
- v0 interpreter currently uses error-as-value for I/O.

3) Blocks:
- v0 interpreter uses indentation-sensitive parsing for a small subset.
