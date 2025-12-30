#include "parser.h"
#include "interp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* slurp_file(const char* path, size_t* out_len) {
  FILE* f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return NULL; }
  char* buf = (char*)malloc((size_t)n + 2);
  if (!buf) { fclose(f); return NULL; }
  size_t r = fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[r] = '\n';
  buf[r+1] = '\0';
  if (out_len) *out_len = r + 1;
  return buf;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <file.astr>\n", argv[0]);
    return 2;
  }
  size_t len = 0;
  char* src = slurp_file(argv[1], &len);
  if (!src) {
    fprintf(stderr, "error: could not read file: %s\n", argv[1]);
    return 2;
  }

  ParseError err;
  Program p = parse_source(src, len, &err);
  if (err.has_error) {
    fprintf(stderr, "parse error at %zu:%zu: %s\n", err.line, err.col, err.message);
    free(src);
    return 1;
  }

  Env env;
  env_init(&env);

  char rerr[256] = {0};
  bool ok = run_program(&p, &env, rerr, sizeof(rerr));
  if (!ok) {
    fprintf(stderr, "runtime error: %s\n", rerr[0] ? rerr : "unknown");
    program_free(&p);
    env_free(&env);
    free(src);
    return 1;
  }

  env_free(&env);
  program_free(&p);
  free(src);
  return 0;
}
