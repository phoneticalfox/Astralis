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
} Env;

void env_init(Env* e);
void env_free(Env* e);

Value env_get(const Env* e, const char* name, size_t n);
bool env_set(Env* e, const char* name, size_t n, const Value* v, bool is_lock, char* errbuf, size_t errbuf_n);

Value eval_expr(const Expr* e, Env* env);

bool run_program(const Program* p, Env* env, char* errbuf, size_t errbuf_n);
