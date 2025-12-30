// bindings/README
// This directory stores generated FFI bindings.
//
// A minimal `astrac c-import` prototype lives at tools/astrac_c_import.py.
// It uses Clang's JSON AST output to build a foreign module from a header.
// Usage:
//   python tools/astrac_c_import.py <header> --link <lib> -o bindings/<name>.astr
//
// Example:
//   python tools/astrac_c_import.py examples/ffi/simple_math.h --link c -o bindings/simple_math.astr
