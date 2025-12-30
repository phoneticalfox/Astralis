#include "interp.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char* dup_n(const char* s, size_t n) {
  char* out = (char*)malloc(n + 1);
  if (!out) return NULL;
  memcpy(out, s, n);
  out[n] = '\0';
  return out;
}

void env_init(Env* e) {
  e->items = NULL; e->count = 0; e->cap = 0; e->parent = NULL;
}

Env* env_push(Env* parent) {
  Env* e = (Env*)calloc(1, sizeof(Env));
  env_init(e);
  e->parent = parent;
  return e;
}

void env_pop(Env* env) {
  env_free(env);
  free(env);
}

void env_free(Env* e) {
  if (!e) return;
  for (size_t i = 0; i < e->count; i++) {
    if (e->items[i].value.type == VAL_FUNC && e->items[i].value.func) {
      free(e->items[i].value.func);
      e->items[i].value.func = NULL;
    }
    free(e->items[i].name);
    value_free(&e->items[i].value);
  }
  free(e->items);
  e->items = NULL; e->count = 0; e->cap = 0; e->parent = NULL;
}

static Binding* find_binding(Env* e, const char* name, size_t n) {
  for (Env* cur = e; cur; cur = cur->parent) {
    for (size_t i = 0; i < cur->count; i++) {
      if (strlen(cur->items[i].name) == n && memcmp(cur->items[i].name, name, n) == 0) {
        return &cur->items[i];
      }
    }
  }
  return NULL;
}

static Binding* find_local_binding(Env* e, const char* name, size_t n) {
  for (size_t i = 0; i < e->count; i++) {
    if (strlen(e->items[i].name) == n && memcmp(e->items[i].name, name, n) == 0) {
      return &e->items[i];
    }
  }
  return NULL;
}

Value env_get(const Env* e, const char* name, size_t n) {
  Binding* b = find_binding((Env*)e, name, n);
  return b ? value_copy(&b->value) : value_error("undefined variable", strlen("undefined variable"));
}

static bool env_set_internal(Env* e, const char* name, size_t n, const Value* v, bool is_lock, char* errbuf, size_t errbuf_n, bool only_local) {
  Binding* existing = only_local ? find_local_binding(e, name, n) : find_binding(e, name, n);
  if (existing) {
    if (existing->is_lock) { snprintf(errbuf, errbuf_n, "cannot assign to locked binding"); return false; }
    value_free(&existing->value);
    existing->value = value_copy(v);
    return true;
  }
  if (e->count + 1 > e->cap) {
    size_t nc = e->cap ? e->cap * 2 : 16;
    e->items = (Binding*)realloc(e->items, nc * sizeof(Binding));
    e->cap = nc;
  }
  Binding nb;
  nb.name = dup_n(name, n);
  nb.value = value_copy(v);
  nb.is_lock = is_lock;
  e->items[e->count++] = nb;
  return true;
}

bool env_set(Env* e, const char* name, size_t n, const Value* v, bool is_lock, char* errbuf, size_t errbuf_n) {
  return env_set_internal(e, name, n, v, is_lock, errbuf, errbuf_n, false);
}

bool env_define_local(Env* e, const char* name, size_t n, const Value* v, bool is_lock, char* errbuf, size_t errbuf_n) {
  return env_set_internal(e, name, n, v, is_lock, errbuf, errbuf_n, true);
}

static Value add_values(const Value* a, const Value* b) {
  if (a->type == VAL_INT && b->type == VAL_INT) {
    return value_int(a->i + b->i);
  }
  char* sa = value_to_cstring(a);
  char* sb = value_to_cstring(b);
  if (!sa || !sb) {
    if (sa) free(sa);
    if (sb) free(sb);
    return value_error("out of memory", strlen("out of memory"));
  }
  size_t na = strlen(sa), nb = strlen(sb);
  char* out = (char*)malloc(na + nb + 1);
  if (!out) {
    free(sa); free(sb);
    return value_error("out of memory", strlen("out of memory"));
  }
  memcpy(out, sa, na);
  memcpy(out + na, sb, nb);
  out[na + nb] = '\0';
  free(sa); free(sb);
  Value v; v.type = VAL_STRING; v.i = 0; v.s = out; v.func = NULL; v.builtin = NULL; v.b = false;
  return v;
}

static Value call_function(const Function* fn, const Value* args, size_t argc, Env* env, char* errbuf, size_t errbuf_n);

static Value eval_call(const CallExpr* call, Env* env, char* errbuf, size_t errbuf_n) {
  if (!call) return value_error("null call", strlen("null call"));
  Value callee = eval_expr(call->callee, env);
  if (callee.type == VAL_ERROR) return callee;
  Value* argv = NULL;
  if (call->arg_count) {
    argv = (Value*)calloc(call->arg_count, sizeof(Value));
    for (size_t i = 0; i < call->arg_count; i++) {
      argv[i] = eval_expr(call->args[i], env);
      if (argv[i].type == VAL_ERROR) {
        for (size_t j = 0; j < i; j++) value_free(&argv[j]);
        Value err = argv[i];
        free(argv);
        value_free(&callee);
        return err;
      }
    }
  }

  char local_err[256] = {0};
  char* errp = errbuf ? errbuf : local_err;
  size_t errn = errbuf ? errbuf_n : sizeof(local_err);

  Value result = value_error("unsupported call", strlen("unsupported call"));
  if (callee.type == VAL_BUILTIN) {
    result = callee.builtin->fn(argv, call->arg_count);
  } else if (callee.type == VAL_FUNC) {
    result = call_function(callee.func, argv, call->arg_count, env, errp, errn);
  }

  for (size_t i = 0; i < call->arg_count; i++) value_free(&argv[i]);
  free(argv);
  value_free(&callee);
  return result;
}

Value eval_expr(const Expr* e, Env* env) {
  if (!e) return value_error("null expr", strlen("null expr"));
  switch (e->type) {
    case EXPR_LITERAL:
      return value_copy(&e->lit);
    case EXPR_IDENT:
      return env_get(env, e->tok.start, e->tok.length);
    case EXPR_GROUP:
      return eval_expr(e->left, env);
    case EXPR_BINARY: {
      Value l = eval_expr(e->left, env);
      if (l.type == VAL_ERROR) return l;
      Value r = eval_expr(e->right, env);
      if (r.type == VAL_ERROR) { value_free(&l); return r; }
      Value out = add_values(&l, &r);
      value_free(&l);
      value_free(&r);
      return out;
    }
    case EXPR_CALL:
      return eval_call(&e->call, env, NULL, 0);
    default:
      return value_error("unknown expr", strlen("unknown expr"));
  }
}

typedef struct ExecState {
  bool returned;
  bool broke;
  bool cont;
  Value ret;
} ExecState;

static bool exec_block(const Block* b, Env* env, ExecState* st, char* errbuf, size_t errbuf_n);

static bool exec_stmt(const Stmt* s, Env* env, ExecState* st, char* errbuf, size_t errbuf_n) {
  switch (s->type) {
    case STMT_SHOW: {
      Value v = eval_expr(s->expr, env);
      if (v.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", v.s ? v.s : "error"); value_free(&v); return false; }
      rt_show(&v);
      value_free(&v);
      return true;
    }
    case STMT_WARN: {
      Value v = eval_expr(s->expr, env);
      if (v.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", v.s ? v.s : "error"); value_free(&v); return false; }
      rt_warn(&v);
      value_free(&v);
      return true;
    }
    case STMT_SET:
    case STMT_LOCK: {
      Value v = eval_expr(s->expr, env);
      if (v.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", v.s ? v.s : "error"); value_free(&v); return false; }
      bool ok = env_set(env, s->name.start, s->name.length, &v, s->type == STMT_LOCK, errbuf, errbuf_n);
      value_free(&v);
      return ok;
    }
    case STMT_IF: {
      Value cond = eval_expr(s->expr, env);
      if (cond.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", cond.s ? cond.s : "error"); value_free(&cond); return false; }
      bool truth = value_is_truthy(&cond);
      value_free(&cond);
      if (truth && s->block) return exec_block(s->block, env, st, errbuf, errbuf_n);
      if (!truth && s->else_block) return exec_block(s->else_block, env, st, errbuf, errbuf_n);
      return true;
    }
    case STMT_LOOP_FOREVER: {
      while (1) {
        if (!exec_block(s->block, env, st, errbuf, errbuf_n)) return false;
        if (st->returned) return true;
        if (st->broke) { st->broke = false; return true; }
        if (st->cont) { st->cont = false; continue; }
      }
    }
    case STMT_REPEAT: {
      Value start = eval_expr(s->expr, env);
      if (start.type != VAL_INT) { snprintf(errbuf, errbuf_n, "repeat start must be int"); value_free(&start); return false; }
      Value end = eval_expr(s->expr_b, env);
      if (end.type != VAL_INT) { snprintf(errbuf, errbuf_n, "repeat end must be int"); value_free(&start); value_free(&end); return false; }
      for (long i = start.i; i <= end.i; i++) {
        Value iv = value_int(i);
        if (!env_define_local(env, s->loop_var.start, s->loop_var.length, &iv, false, errbuf, errbuf_n)) { value_free(&iv); value_free(&start); value_free(&end); return false; }
        value_free(&iv);
        if (!exec_block(s->block, env, st, errbuf, errbuf_n)) { value_free(&start); value_free(&end); return false; }
        if (st->returned) { value_free(&start); value_free(&end); return true; }
        if (st->broke) { st->broke = false; break; }
        if (st->cont) { st->cont = false; continue; }
      }
      value_free(&start); value_free(&end);
      return true;
    }
    case STMT_DEFINE: {
      Function* fn = (Function*)calloc(1, sizeof(Function));
      fn->name = s->name;
      fn->params = s->params;
      fn->param_count = s->param_count;
      fn->body = s->block;
      fn->closure = env;
      Value fv = value_func(fn);
      bool ok = env_define_local(env, s->name.start, s->name.length, &fv, true, errbuf, errbuf_n);
      value_free(&fv);
      return ok;
    }
    case STMT_RETURN: {
      st->returned = true;
      if (s->expr) st->ret = eval_expr(s->expr, env);
      else st->ret = value_null();
      return true;
    }
    case STMT_BREAK: st->broke = true; return true;
    case STMT_CONTINUE: st->cont = true; return true;
    case STMT_EXPR: {
      Value v = eval_expr(s->expr, env);
      if (v.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", v.s ? v.s : "error"); value_free(&v); return false; }
      value_free(&v);
      return true;
    }
    default:
      snprintf(errbuf, errbuf_n, "unsupported statement (seed0)");
      return false;
  }
}

static bool exec_block(const Block* b, Env* env, ExecState* st, char* errbuf, size_t errbuf_n) {
  if (!b) return true;
  for (size_t i = 0; i < b->count; i++) {
    if (!exec_stmt(&b->stmts[i], env, st, errbuf, errbuf_n)) return false;
    if (st->returned || st->broke || st->cont) return true;
  }
  return true;
}

static Value call_function(const Function* fn, const Value* args, size_t argc, Env* env, char* errbuf, size_t errbuf_n) {
  if (!fn) return value_error("null function", strlen("null function"));
  if (argc != fn->param_count) return value_error("arity mismatch", strlen("arity mismatch"));
  Env* frame = env_push(fn->closure ? fn->closure : env);
  for (size_t i = 0; i < argc; i++) {
    if (!env_define_local(frame, fn->params[i].start, fn->params[i].length, &args[i], false, errbuf, errbuf_n)) {
      env_pop(frame);
      return value_error(errbuf, strlen(errbuf));
    }
  }
  ExecState st = {0};
  char local_err[256] = {0};
  bool ok = exec_block(fn->body, frame, &st, local_err, sizeof(local_err));
  env_pop(frame);
  if (!ok) return value_error(local_err[0] ? local_err : "error", strlen(local_err[0] ? local_err : "error"));
  if (st.returned) return st.ret;
  return value_null();
}

// Builtins
static Value builtin_ask(const Value* args, size_t count) {
  if (count != 1) return value_error("ask expects 1 arg", strlen("ask expects 1 arg"));
  return rt_ask(&args[0]);
}

static const Builtin BUILTIN_ASK = {"ask", 1, builtin_ask};

bool run_program(const Program* p, Env* env, char* errbuf, size_t errbuf_n) {
  // preload builtins
  Value askv = value_builtin(&BUILTIN_ASK);
  if (!env_define_local(env, "ask", strlen("ask"), &askv, true, errbuf, errbuf_n)) {
    value_free(&askv);
    return false;
  }
  value_free(&askv);

  ExecState st = {0};
  return exec_block(&p->block, env, &st, errbuf, errbuf_n);
}
