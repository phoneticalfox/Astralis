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
  e->items = NULL; e->count = 0; e->cap = 0;
}
void env_free(Env* e) {
  if (!e) return;
  for (size_t i = 0; i < e->count; i++) {
    free(e->items[i].name);
    value_free(&e->items[i].value);
  }
  free(e->items);
  e->items = NULL; e->count = 0; e->cap = 0;
}

static long name_hash(const char* s) {
  // djb2
  unsigned long h = 5381;
  for (; *s; s++) h = ((h << 5) + h) + (unsigned char)(*s);
  return (long)h;
}

static Binding* find_binding(const Env* e, const char* name, size_t n) {
  (void)name_hash;
  for (size_t i = 0; i < e->count; i++) {
    if (strlen(e->items[i].name) == n && memcmp(e->items[i].name, name, n) == 0) {
      return &((Env*)e)->items[i];
    }
  }
  return NULL;
}

Value env_get(const Env* e, const char* name, size_t n) {
  Binding* b = find_binding(e, name, n);
  if (!b) return value_error("undefined variable", strlen("undefined variable"));
  return value_copy(&b->value);
}

bool env_set(Env* e, const char* name, size_t n, const Value* v, bool is_lock, char* errbuf, size_t errbuf_n) {
  Binding* b = find_binding(e, name, n);
  if (b) {
    if (b->is_lock) {
      snprintf(errbuf, errbuf_n, "cannot assign to locked binding");
      return false;
    }
    value_free(&b->value);
    b->value = value_copy(v);
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

static Value add_values(const Value* a, const Value* b) {
  if (a->type == VAL_INT && b->type == VAL_INT) {
    return value_int(a->i + b->i);
  }
  // stringify concat
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
  Value v; v.type = VAL_STRING; v.i = 0; v.s = out;
  return v;
}

Value eval_expr(const Expr* e, Env* env) {
  if (!e) return value_error("null expr", strlen("null expr"));
  switch (e->type) {
    case EXPR_LITERAL:
      return value_copy(&e->lit);
    case EXPR_IDENT:
      return env_get(env, e->tok.start, e->tok.length);
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
    case EXPR_CALL: {
      // only ask(prompt) supported
      if (e->call.callee.type != TOK_ASK) {
        return value_error("unsupported call", strlen("unsupported call"));
      }
      Value prompt = eval_expr(e->call.arg, env);
      if (prompt.type == VAL_ERROR) return prompt;
      Value ans = rt_ask(&prompt);
      value_free(&prompt);
      return ans;
    }
    default:
      return value_error("unknown expr", strlen("unknown expr"));
  }
}

bool run_program(const Program* p, Env* env, char* errbuf, size_t errbuf_n) {
  for (size_t i = 0; i < p->count; i++) {
    const Stmt* s = &p->stmts[i];
    switch (s->type) {
      case STMT_SHOW: {
        Value v = eval_expr(s->expr, env);
        if (v.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", v.s ? v.s : "error"); value_free(&v); return false; }
        rt_show(&v);
        value_free(&v);
        break;
      }
      case STMT_WARN: {
        Value v = eval_expr(s->expr, env);
        if (v.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", v.s ? v.s : "error"); value_free(&v); return false; }
        rt_warn(&v);
        value_free(&v);
        break;
      }
      case STMT_SET:
      case STMT_LOCK: {
        Value v = eval_expr(s->expr, env);
        if (v.type == VAL_ERROR) { snprintf(errbuf, errbuf_n, "%s", v.s ? v.s : "error"); value_free(&v); return false; }
        bool ok = env_set(env, s->name.start, s->name.length, &v, s->type == STMT_LOCK, errbuf, errbuf_n);
        value_free(&v);
        if (!ok) return false;
        break;
      }
      default:
        snprintf(errbuf, errbuf_n, "unsupported statement (seed0)");
        return false;
    }
  }
  return true;
}
