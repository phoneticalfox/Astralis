#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* dup_n(const char* s, size_t n) {
  char* out = (char*)malloc(n + 1);
  if (!out) return NULL;
  memcpy(out, s, n);
  out[n] = '\0';
  return out;
}

void rt_show(const Value* v) {
  char* s = value_to_cstring(v);
  if (!s) return;
  fputs(s, stdout);
  fputc('\n', stdout);
  free(s);
}

void rt_warn(const Value* v) {
  char* s = value_to_cstring(v);
  if (!s) return;
  fputs("warning: ", stderr);
  fputs(s, stderr);
  fputc('\n', stderr);
  free(s);
}

Value rt_ask(const Value* prompt) {
  char* p = value_to_cstring(prompt);
  if (!p) p = dup_n("", 0);
  fputs(p, stdout);
  fflush(stdout);
  free(p);

  char buf[4096];
  if (!fgets(buf, sizeof(buf), stdin)) {
    return value_error("stdin read failed", strlen("stdin read failed"));
  }
  // strip trailing newline
  size_t n = strlen(buf);
  while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';
  return value_string(buf, n);
}
