#pragma once
#include "lexer.h"
#include "value.h"
#include <stdbool.h>

typedef enum ExprType {
  EXPR_LITERAL = 0,
  EXPR_IDENT,
  EXPR_BINARY,
  EXPR_CALL
} ExprType;

typedef struct Expr Expr;

typedef enum BinOp {
  BIN_ADD = 0
} BinOp;

typedef struct CallExpr {
  Token callee;      // identifier/keyword token
  Expr* arg;         // single-arg calls in seed0 (ask)
} CallExpr;

struct Expr {
  ExprType type;
  Token tok;         // for ident or literal token (literal: STRING/NUMBER)
  Value lit;         // if literal
  BinOp op;
  Expr* left;
  Expr* right;
  CallExpr call;
};

typedef enum StmtType {
  STMT_SHOW = 0,
  STMT_WARN,
  STMT_SET,
  STMT_LOCK,
  STMT_UNSUPPORTED
} StmtType;

typedef struct Stmt {
  StmtType type;
  Token name;        // for set/lock
  Expr* expr;        // expression for show/warn/set/lock
  size_t line;
} Stmt;

typedef struct Program {
  Stmt* stmts;
  size_t count;
  size_t cap;
} Program;

typedef struct ParseError {
  bool has_error;
  char message[256];
  size_t line;
  size_t col;
} ParseError;

void program_free(Program* p);

Program parse_source(const char* src, size_t len, ParseError* err);
