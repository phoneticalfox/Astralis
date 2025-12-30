# Astralis FFI v0 Spec (Clang-powered C ABI)

Goal: **Day one** ability to call C ABI functions from Astralis, at scale, by importing
C headers via **Clang** and generating Astralis bindings.

This spec defines:
- the ABI baseline(s)
- the foreign declaration surface
- the type mapping layer (Astralis ↔ C)
- linking/loading behavior
- safety rules at the boundary
- the `astrac c-import` workflow

---

## 1. Supported ABIs (v0)

v0 must specify at least one concrete target. Recommended minimal baseline:

- `x86_64-linux-gnu` (System V AMD64 ABI)

Additional targets can be added later via `target triples`.

## 2. Required “C types” module in Astralis

Provide a canonical module (name is flexible; examples use `c::`):

### 2.1 Integer & float primitives
- `c.i8,  c.i16,  c.i32,  c.i64`
- `c.u8,  c.u16,  c.u32,  c.u64`
- `c.f32, c.f64`

### 2.2 Platform-sized
- `c.isize`
- `c.usize`

### 2.3 Bool
- `c.bool` (define representation: 8-bit `_Bool` on C99+)

### 2.4 Pointers
- `c.ptr<T>`
- `c.void_ptr` (alias `c.ptr<c.void>` if `c.void` exists)

### 2.5 C strings
- `c.cstring` : NUL-terminated `char*`
- `c.const_cstring` : `const char*`

### 2.6 Struct / union layout
Provide a layout annotation:
- `layout c` or `repr(c)` equivalent.
Rules:
- field order preserved
- alignment/padding matches C compiler for the target
- unions share storage starting at offset 0

### 2.7 Enums
- enums map to an explicit integer type:
  - default: `c.i32` unless Clang reports a different underlying type

---

## 3. Foreign declarations (source-level surface)

Astralis needs a way to declare that a symbol is foreign (C ABI):

### 3.1 Foreign function
Example:
```
foreign module c::stdio links "c":
  foreign define puts(s: c.const_cstring) -> c.i32
```

Required elements:
- module namespace
- `links "<libname>"` (for the linker / loader)
- `foreign define <name>(...) -> <ret>`
- calling convention defaults to C ABI for the target

### 3.2 Foreign global
```
foreign declare errno: c.i32
```

### 3.3 Function pointers / callbacks
```
foreign type callback_t as c.fnptr(c.i32, c.ptr<c.void>) -> c.i32
```

Minimum v0 rule:
- callbacks must reference **named functions**, not closures
- calling convention is C

### 3.4 Variadic functions
v0 must choose:
- **Option A:** support C varargs ABI for the v0 target
- **Option B (acceptable):** disallow direct varargs calls; require wrapper functions
The importer must detect varargs and either:
- mark them as `unsupported in v0`, or
- emit a wrapper request.

---

## 4. `astrac c-import` (Clang importer)

### 4.1 CLI shape
Proposed:
```
astrac c-import <header.h> \
  --target x86_64-linux-gnu \
  -I <include_dir>... \
  -D <macro>... \
  --link <libname>... \
  -o bindings/<module>.astr
```

### 4.2 Behavior
- Invoke Clang with the given target/include/defines.
- Parse the header into an AST.
- Emit an Astralis file containing:
  - `foreign module ...`
  - type definitions (struct/union/enum/typedef)
  - `foreign define` prototypes for functions
  - `foreign declare` for exported globals (when possible)

### 4.3 Macro handling
Macros are not linkable symbols. Importer must:
- For object-like numeric/string macros: emit Astralis constants where safe.
- For function-like macros / complex expressions: emit stubs or require wrapper generation.

### 4.4 Wrappers (optional but recommended)
Provide an optional flag:
- `--emit-wrappers wrappers.c` to generate C wrappers for unsupported cases:
  - varargs
  - inline functions
  - complex macros

Wrappers are then compiled/linked alongside the Astralis program.

---

## 5. Linking / loading

### 5.1 Static vs dynamic
v0 must support **dynamic** linking at minimum.

Two modes:

#### Build-time link (recommended default)
- `links "sqlite3"` becomes `-lsqlite3` (platform mapping rules apply)
- include paths for headers are independent of library search paths

#### Runtime load (optional)
Provide a stdlib module:
- `c.dl.open(path)`, `c.dl.sym(handle, name)`

If runtime load is supported, `links` may be interpreted as:
- “load at runtime if present, otherwise error at call”

### 5.2 Toolchain flags
`astrac` should accept:
- `-L <dir>` library search path
- `-l <name>` library name
- `-Wl,...` passthrough (optional)
- per-target selection (required if multiple ABIs supported)

---

## 6. Boundary safety model

### 6.1 Unsafe boundary
FFI calls are inherently unsafe. Provide a marker:
- `unsafe:` block
or
- `trust:` block

Example:
```
unsafe:
  c::stdio::puts(cstr)
```

### 6.2 Strings
Define explicit conversions:
- `string -> c.const_cstring` (temporary, NUL-terminated copy)
- `c.cstring -> string` (copy until NUL)

Define lifetimes:
- temporary cstrings are valid for the duration of the call only.

### 6.3 Memory ownership
Provide standard helpers:
- `c.malloc(size: c.usize) -> c.void_ptr`
- `c.free(ptr: c.void_ptr)`

If a library returns memory that must be freed by a library-specific function,
the binding generator should expose that function and documentation should mark it.

### 6.4 Struct passing
Rules must match C ABI:
- pass-by-value for small structs where ABI requires it
- otherwise pass pointers

The importer should know the correct passing rules from Clang.

---

## 7. Minimal conformance tests (v0)

Ship a small FFI test suite that validates:
- calling a function in libc (`puts`, `strlen`)
- passing strings and getting ints back
- calling a function that takes a callback (small custom C library)
- struct layout roundtrip (custom C helper)

---

## 8. Non-goals for v0
- full C++ name mangling / templates
- automatic memory safety across FFI (FFI remains unsafe)
- perfect macro support without wrappers
