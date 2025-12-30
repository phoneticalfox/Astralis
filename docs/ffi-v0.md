# Astralis FFI v0 Spec (Clang-powered C ABI) — Normative

Goal: **v0 MUST enable calling arbitrary C ABI symbols** from Astralis by importing C headers via **Clang** and generating Astralis bindings.

This spec defines:
- ABI baseline(s)
- foreign declaration surface
- C type mapping layer (Astralis ↔ C)
- linking/loading contract
- boundary safety rules
- `astrac c-import` workflow
- conformance tests

Normative keywords: **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **MAY**.

---

## 0. Terminology

- **C ABI function**: a callable symbol using the platform C calling convention for the selected target.
- **Binding**: an Astralis source module containing `foreign` declarations generated from headers.
- **Target triple**: Clang/LLVM triple identifying platform + ABI (e.g., `x86_64-linux-gnu`).

---

## 1. Supported ABIs (v0)

### 1.1 Required baseline
v0 **MUST** support at least:
- `x86_64-linux-gnu` (System V AMD64 ABI)

### 1.2 Future targets
v0 **MAY** add additional target triples. If multiple targets are supported, `astrac` **MUST** require explicit target selection when it cannot be inferred.

---

## 2. Required C Types Module

Astralis **MUST** provide a canonical module namespace for C ABI types (examples use `c::`).

### 2.1 Integer & float primitives
The following types **MUST** exist and be ABI-exact:

- `c.i8,  c.i16,  c.i32,  c.i64`
- `c.u8,  c.u16,  c.u32,  c.u64`
- `c.f32, c.f64`

### 2.2 Platform-sized
- `c.isize`
- `c.usize`

`c.usize` **MUST** match `sizeof(size_t)` for the target.

### 2.3 Bool
- `c.bool` **MUST** match C99 `_Bool` for the target.
- Representation for `x86_64-linux-gnu`: 8-bit.

### 2.4 Pointers
- `c.ptr<T>` **MUST** represent a machine pointer for the target.
- `c.void` **MAY** exist; if present, `c.void_ptr` **MUST** be aliasable to `c.ptr<c.void>`.
- If `c.void` does not exist, `c.void_ptr` **MUST** still exist.

### 2.5 C strings
- `c.cstring` **MUST** represent `char*`.
- `c.const_cstring` **MUST** represent `const char*`.

### 2.6 Struct / union layout
Astralis **MUST** support a C layout annotation:
- `layout c` or `repr(c)` (spelling is up to Astralis; semantics are not)

Rules:
- Field order **MUST** be preserved.
- Alignment/padding **MUST** match Clang’s layout for the selected target.
- A union’s fields **MUST** share storage starting at offset 0.
- Bitfields **MAY** be unsupported in v0; if unsupported, the importer **MUST** emit a clear “unsupported” note.

### 2.7 Enums
- Enums **MUST** map to a concrete integer type.
- Default **MUST** be `c.i32` unless Clang reports a different underlying type.

---

## 3. Foreign Declarations (source-level surface)

### 3.1 Foreign modules
Astralis **MUST** allow grouping foreign declarations under a module namespace and associating them with one or more libraries.

Example:
```astralis
foreign module c::stdio links "c":
  foreign define puts(s: c.const_cstring) -> c.i32
```

Requirements:
- `foreign module <namespace> ... :` **MUST** introduce a declaration block.
- `links "<libname>"` **MUST** declare a link dependency for the module.
- Calling convention **MUST** default to C ABI for the selected target.

`links` **MAY** accept multiple libraries:
```astralis
foreign module c::x links ["c", "m"]:
  ...
```

### 3.2 Foreign functions
Syntax:
```astralis
foreign define <name>(<params...>) -> <ret>
```

Rules:
- Parameter and return types **MUST** be expressible using the C types module and any imported `repr(c)` structs/unions/enums/typedefs.
- A foreign function **MAY** specify an explicit symbol name if the Astralis name differs:
```astralis
foreign define strerror(err: c.i32) -> c.const_cstring symbol "strerror"
```
- Overloading **MUST NOT** apply to foreign functions (C has no overload by name).

### 3.3 Foreign globals
Astralis **MAY** support foreign globals:
```astralis
foreign declare errno: c.i32
```

Constraints:
- v0 **MUST NOT** claim to support TLS-backed globals unless it does so correctly on the target.
- The importer **SHOULD** prefer importing accessor functions when a “global” is actually TLS (e.g., emit `__errno_location()` patterns where applicable) or mark as unsupported.

### 3.4 Function pointers / callbacks
Astralis **MUST** support a C function pointer type:
```astralis
c.fnptr(<param types...>) -> <ret type>
```

Example:
```astralis
foreign type qsort_cmp as c.fnptr(c.ptr<c.void>, c.ptr<c.void>) -> c.i32
```

Minimum v0 rules:
- Callbacks passed to C **MUST** be **named functions**, not closures.
- Calling convention for callbacks **MUST** be C ABI.

### 3.5 Variadic functions (v0 decision)
v0 **MUST** treat direct calls to variadic functions as **unsupported**.

Importer behavior:
- If a function is variadic, importer **MUST** either:
  - (a) mark it `unsupported in v0 (varargs)` OR
  - (b) require wrapper emission via `--emit-wrappers`.

---

## 4. `astrac c-import` (Clang importer)

### 4.1 CLI
`astrac` **MUST** provide:
```bash
astrac c-import <header.h> \
  --target x86_64-linux-gnu \
  -I <include_dir>... \
  -D <macro>... \
  --link <libname>... \
  -o bindings/<module>.astr
```

Rules:
- `--target` **MUST** select the ABI.
- `-I` and `-D` **MUST** be forwarded to Clang.
- `--link` **MUST** populate `links` metadata in the emitted module(s).

### 4.2 Output
Importer **MUST** emit valid Astralis source that contains:
- `foreign module ...`
- all required type declarations (struct/union/enum/typedef mappings)
- `foreign define` prototypes for functions
- `foreign declare` for supported globals (when possible and correct)

Output **SHOULD** be deterministic for a given:
- header path
- target
- include paths/defines
- clang version (best-effort; small diffs are acceptable but should be minimized)

### 4.3 C constructs coverage (v0 minimum)
Importer **MUST** handle:
- `typedef` chains
- pointers and `const` qualifiers
- structs/unions (non-bitfield required)
- enums (with underlying type)
- function prototypes (non-variadic required)
- function pointer typedefs

Importer **MAY** initially mark unsupported:
- bitfields
- flexible array members
- vector extensions
- complex attributes that change ABI

### 4.4 Macro handling
Macros are not linkable symbols.

Importer **MUST**:
- For object-like numeric macros: emit Astralis constants when the value can be safely evaluated for the target.
- For string literal macros: emit Astralis string constants.
- For function-like macros or complex expressions: mark as `macro requires wrapper` or `unsupported in v0`.

### 4.5 Wrappers (recommended)
Importer **SHOULD** support:
```bash
astrac c-import ... --emit-wrappers wrappers.c
```

Wrappers **MUST** be generated for:
- variadic functions (required for v0)
- inline functions that are not emitted as symbols
- complex macros when feasible

---

## 5. Linking / Loading

### 5.1 Build-time linking (required)
v0 **MUST** support build-time dynamic linking.

Rules:
- `links "sqlite3"` **MUST** map to `-lsqlite3` (platform naming rules apply).
- `astrac` **MUST** accept:
  - `-L <dir>` library search path
  - `-l <name>` library name
- Header include paths **MUST** be independent from library search paths.

### 5.2 Runtime loading (optional)
v0 **MAY** additionally support runtime dynamic loading via a stdlib module:
- `c.dl.open(path) -> handle`
- `c.dl.sym(handle, name) -> c.void_ptr`

If runtime loading is supported:
- `links` **MAY** be interpreted as “load at runtime” only when explicitly requested (do not silently change link-time behavior).

---

## 6. Boundary Safety Model

### 6.1 Unsafe boundary (required)
FFI calls are inherently unsafe.

Astralis **MUST** require that calling a `foreign define` occurs inside an `unsafe:` block (or equivalent marker).

Example:
```astralis
unsafe:
  c::stdio::puts(msg)
```

### 6.2 Strings (required rules)
Astralis **MUST** define explicit conversions:
- `string -> c.const_cstring`: creates a temporary NUL-terminated buffer
- `c.cstring / c.const_cstring -> string`: copies until NUL

Lifetime rule:
- Temporary cstrings produced from Astralis strings are valid **only for the duration of the call**.

### 6.3 Memory ownership helpers
Astralis **SHOULD** expose:
- `c.malloc(size: c.usize) -> c.void_ptr`
- `c.free(ptr: c.void_ptr)`

If a library returns memory requiring library-specific free functions, the binding **MUST** expose those functions, and documentation **SHOULD** annotate ownership expectations.

### 6.4 Struct passing
ABI rules **MUST** match the selected target. Importer **SHOULD** rely on Clang-provided ABI knowledge for:
- pass-by-value vs indirect passing
- return-by-value handling

---

## 7. Minimal Conformance Tests (required)

Repo **MUST** include an automated FFI test suite validating at least:

1) libc call:
- `puts` and `strlen` via imported headers

2) string passing:
- passing Astralis strings to `c.const_cstring`
- receiving `c.const_cstring` and converting to Astralis `string`

3) callback:
- a small custom C helper library that takes a callback and calls it

4) struct layout roundtrip:
- custom C helper that fills a struct and returns it / validates fields

CI **SHOULD** run the conformance suite on the v0 baseline target.

---

## 8. Non-goals for v0
- C++ name mangling/templates
- automatic memory safety across FFI (FFI remains unsafe)
- perfect macro support without wrappers
- direct variadic calls (wrapper-required in v0)
