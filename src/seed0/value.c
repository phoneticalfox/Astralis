#include "value.h"
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

Value value_null(void) {
  Value v; v.type = VAL_NULL; v.i = 0; v.s = NULL; return v;
}
Value value_int(long x) {
  Value v; v.type = VAL_INT; v.i = x; v.s = NULL; return v;
}
Value value_string(const char* s, size_t n) {
  Value v; v.type = VAL_STRING; v.i = 0; v.s = dup_n(s, n); return v;
}
Value value_error(const char* s, size_t n) {
  Value v; v.type = VAL_ERROR; v.i = 0; v.s = dup_n(s, n); return v;
}
Value value_bool(bool b) {
  Value v; v.type = VAL_BOOL; v.b = b; v.s = NULL; v.i = 0; v.func = NULL; v.builtin = NULL; return v;
}
Value value_func(struct Function* fn) {
  Value v; v.type = VAL_FUNC; v.func = fn; v.s = NULL; v.i = 0; v.builtin = NULL; return v;
}
Value value_builtin(const struct Builtin* b) {
  Value v; v.type = VAL_BUILTIN; v.builtin = b; v.s = NULL; v.i = 0; v.func = NULL; return v;
}

void value_free(Value* v) {
  if (!v) return;
  if (v->s) free(v->s);
  v->s = NULL;
  v->type = VAL_NULL;
  v->i = 0;
  v->b = false;
  v->func = NULL;
  v->builtin = NULL;
}

Value value_copy(const Value* v) {
  if (!v) return value_null();
  Value out;
  out.type = v->type;
  out.i = v->i;
  out.s = NULL;
  out.b = v->b;
  out.func = v->func;
  out.builtin = v->builtin;
  if ((v->type == VAL_STRING || v->type == VAL_ERROR) && v->s) {
    out.s = dup_n(v->s, strlen(v->s));
  }
  return out;
}

char* value_to_cstring(const Value* v) {
  if (!v) return dup_n("null", 4);
  if (v->type == VAL_NULL) return dup_n("null", 4);
  if (v->type == VAL_INT) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", v->i);
    return dup_n(buf, strlen(buf));
  }
  if (v->type == VAL_BOOL) {
    return dup_n(v->b ? "true" : "false", v->b ? 4 : 5);
  }
  if (v->type == VAL_STRING) {
    if (!v->s) return dup_n("", 0);
    return dup_n(v->s, strlen(v->s));
  }
  if (v->type == VAL_ERROR) {
    if (!v->s) return dup_n("error", 5);
    // prefix
    const char* p = "error: ";
    size_t pn = strlen(p);
    size_t sn = strlen(v->s);
    char* out = (char*)malloc(pn + sn + 1);
    if (!out) return NULL;
    memcpy(out, p, pn);
    memcpy(out + pn, v->s, sn);
    out[pn + sn] = '\0';
    return out;
  }
  if (v->type == VAL_FUNC) {
    return dup_n("<function>", strlen("<function>"));
  }
  if (v->type == VAL_BUILTIN) {
    return dup_n("<builtin>", strlen("<builtin>"));
  }
  return dup_n("<?>", 3);
}

bool value_is_truthy(const Value* v) {
  if (!v) return false;
  switch (v->type) {
    case VAL_NULL: return false;
    case VAL_BOOL: return v->b;
    case VAL_INT: return v->i != 0;
    case VAL_STRING: return v->s && v->s[0] != '\0';
    case VAL_ERROR: return false;
    default: return true;
  }
}
