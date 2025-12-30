# Astralis-Lang (Core Spec Scaffold)

This document defines the **language core** (syntax + semantics) needed to implement
a seed compiler/interpreter and grow toward self-hosting.

## 1. File types

- Source: `.astr`
- (Optional later) Bytecode: `.astrb`

## 2. Design constraints

- Keyword-first, **low-punctuation** syntax.
- Blocks use **keywords** and/or `:` + indentation (implementation may choose one canonical approach).
- Readability prioritized; symbols are allowed where they reduce ambiguity.

## 3. Lexicon (reserved words)

### Declarations
- `set` — declare/assign mutable binding
- `lock` — declare immutable binding

### Memory model markers (v0 surface)
- `own` — owned value
- `borrow` — borrowed reference
  - `borrow read-only <name>`
  - `borrow read-write <name>`

> Note: borrow-checking rules are not fully specified yet; keywords are reserved and semantics are staged.

### Control flow
- `if … then … otherwise …`
- `repeat … from … to …`
- `loop forever`
- `break`, `continue`

### Functions & modules
- `define` — function definition
- `start with <thing> from <module>` — import
- `module <name>:` — declare module namespace

### Error handling
- `try … otherwise …`
- `on error:` — alternate handler block (optional)

### I/O
- `show <expr>` — core output primitive
- `say <expr>` — alias for `show`
- `warn <expr>` — warning output
- `ask(<prompt>)` — read a line (returns string)

### Program structure
- `when program starts:`
- `when program ends:`
- `define main:` (entry shorthand)

### OO / “classes” (reserved; staged)
- `blueprint <Name>(...)` — class/object blueprint

## 4. Lexical rules

### Comments
- Line comment: `// ...` (to end of line)

### Literals
- Strings: double quotes `"like this"`
- Numbers: decimal integers (float support staged)

### Operators (initial)
- arithmetic: `+ - * /`
- comparisons: `== != < <= > >=`
- boolean ops: `and or not` (optional; can be staged)

### Identifiers
- ASCII letters, digits, `_`, with a leading letter or `_`.

## 5. Core statements (AST-level)

### 5.1 Assignment
```
set x to 10
lock PI to 3.14159
```

### 5.2 Output
```
show "Hello"
say "Hello"
warn "Careful"
```

### 5.3 Input
```
set name to ask("name> ")
```

### 5.4 Conditionals (block form)
```
if x > 5 then
  show "big"
otherwise
  show "small"
```

### 5.5 Conditional expression (inline form, optional early)
```
set label to "Big" if x > 10 otherwise "Small"
```

### 5.6 Loops
Bounded:
```
repeat i from 1 to 3 ->
  show i
```

Infinite:
```
loop forever ->
  ...
```

### 5.7 Error handling
```
try risky() otherwise
  warn "failed"
```

## 6. Function definitions

Two competing “body connectors” are recognized in the design notes:

- Symbol form: `->` (present in examples)
- Word form: `as` (preferred for prose flow)

A pragmatic v0 approach:
- accept both in the parser
- standardize on one later

Examples:
```
define greet(name) -> show "Hello, " + name
define greet(name) as show "Hello, " + name
```

## 7. Semantics (staged)

### 7.1 Types
Stated direction: dynamic/runtime type checking (v0), with a path to more type safety later.

Minimum runtime types:
- null
- bool
- int64
- string
- list (staged; required by many programs)
- map/object (staged)

### 7.2 Errors
Two viable v0 models:
- exceptions (try/otherwise catches)
- error-as-a-value (stdlib returns `error` sentinel)

Pick one canonical representation early; keep syntax stable.

### 7.3 `show` lowering contract
`show <expr>` lowers to a runtime call such as:
- `runtime.io.print(value_as_string)`

If implementing a VM, also reserve a bytecode opcode such as:
- `PRINT_STRING`

## 8. Optional “interrobang” feature

- Unicode: `‽` as an emphasis suffix (e.g., `save‽`)
- ASCII fallback: `!important`

Semantics are optional; recommended meaning: “explicitly important / deliberate / high priority.”
This should not be required for core programs to run.

## 9. Out-of-scope for the language core
- full-screen terminal UI APIs
- editor buffer primitives
- graphics/framebuffer primitives
- mobile kana input sugar

Those are extension layers and should not block compiler v0.
