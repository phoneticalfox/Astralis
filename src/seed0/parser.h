#pragma once
#include "lexer.h"
#include "value.h"
#include <stdbool.h>

typedef enum ExprType {
  EXPR_LITERAL = 0,
  EXPR_IDENT,
  EXPR_BINARY,
  EXPR_CALL,
  EXPR_GROUP
} ExprType;

typedef struct Expr Expr;

typedef enum BinOp {
  BIN_ADD = 0
} BinOp;

typedef struct CallExpr {
  Expr* callee;
  Expr** args;
  size_t arg_count;
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
  STMT_IF,
  STMT_LOOP_FOREVER,
  STMT_REPEAT,
  STMT_DEFINE,
  STMT_RETURN,
  STMT_BREAK,
  STMT_CONTINUE,
  STMT_EXPR,
  STMT_UNSUPPORTED
} StmtType;

typedef struct Stmt {
  StmtType type;
  Token name;        // for set/lock
  Expr* expr;        // expression for show/warn/set/lock/return/expr
  Expr* expr_b;      // repeat upper bound
  Token loop_var;    // repeat loop variable
  struct Block* block;     // if/loop/define body
  struct Block* else_block; // otherwise block
  Token* params;     // function parameters
  size_t param_count;
  size_t line;
} Stmt;

typedef struct Block {
  Stmt* stmts;
  size_t count;
  size_t cap;
} Block;

typedef struct Program {
  Block block;
} Program;

typedef struct ParseError {
  bool has_error;
  char message[256];
  size_t line;
  size_t col;
} ParseError;

void program_free(Program* p);

Program parse_source(const char* src, size_t len, ParseError* err);
