#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum ValueType {
  VAL_NULL = 0,
  VAL_INT,
  VAL_STRING,
  VAL_ERROR
} ValueType;

typedef struct Value {
  ValueType type;
  long i;
  char* s;       // heap string for VAL_STRING or VAL_ERROR message
} Value;

Value value_null(void);
Value value_int(long x);
Value value_string(const char* s, size_t n);
Value value_error(const char* s, size_t n);

void value_free(Value* v);
Value value_copy(const Value* v);

// convert to printable string (allocated); caller frees
char* value_to_cstring(const Value* v);
