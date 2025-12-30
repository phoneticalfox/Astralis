# Compiler/Runtime Architecture (Scaffold)

## Seed0 goals
- Implement a minimal interpreter to validate syntax and runtime semantics.
- Keep the AST and runtime “real,” so later stages can reuse structures.

## Later stages
- `astrac` (compiler) will parse to AST, then:
  - emit x86_64 assembly (native)
  - and/or emit `.astrb` bytecode (VM path)

## Suggested pipeline
1. Lexer: source -> tokens
2. Parser: tokens -> AST
3. (Optional) Desugar: normalize syntax variants (`as` vs `->`)
4. Backend:
   - Interpreter (seed0)
   - Codegen (asm)
   - Bytecode emitter + VM
