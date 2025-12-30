#pragma once
#include "parser.h"

typedef struct Binding {
  char* name;
  Value value;
  bool is_lock;
} Binding;

typedef struct Env {
  Binding* items;
  size_t count;
  size_t cap;
  struct Env* parent;
} Env;

typedef struct Function {
  Token name;
  Token* params;
  size_t param_count;
  Block* body;
  Env* closure;
} Function;

typedef struct Builtin {
  const char* name;
  size_t arity;
  Value (*fn)(const Value* args, size_t count);
} Builtin;

void env_init(Env* e);
void env_free(Env* e);
Env* env_push(Env* parent);
void env_pop(Env* env);

Value env_get(const Env* e, const char* name, size_t n);
bool env_set(Env* e, const char* name, size_t n, const Value* v, bool is_lock, char* errbuf, size_t errbuf_n);
bool env_define_local(Env* e, const char* name, size_t n, const Value* v, bool is_lock, char* errbuf, size_t errbuf_n);

Value eval_expr(const Expr* e, Env* env);

bool run_program(const Program* p, Env* env, char* errbuf, size_t errbuf_n);
