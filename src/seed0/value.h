#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum ValueType {
  VAL_NULL = 0,
  VAL_INT,
  VAL_STRING,
  VAL_ERROR,
  VAL_BOOL,
  VAL_FUNC,
  VAL_BUILTIN
} ValueType;

struct Function;
struct Builtin;

typedef struct Value {
  ValueType type;
  long i;
  char* s;       // heap string for VAL_STRING or VAL_ERROR message
  bool b;
  struct Function* func;
  const struct Builtin* builtin;
} Value;

Value value_null(void);
Value value_int(long x);
Value value_string(const char* s, size_t n);
Value value_error(const char* s, size_t n);
Value value_bool(bool b);
Value value_func(struct Function* fn);
Value value_builtin(const struct Builtin* b);

void value_free(Value* v);
Value value_copy(const Value* v);

// convert to printable string (allocated); caller frees
char* value_to_cstring(const Value* v);
bool value_is_truthy(const Value* v);
