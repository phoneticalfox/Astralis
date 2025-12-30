#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void set_error(ParseError* err, size_t line, size_t col, const char* msg) {
  if (!err) return;
  err->has_error = true;
  err->line = line;
  err->col = col;
  snprintf(err->message, sizeof(err->message), "%s", msg);
}

static Expr* expr_new(void) {
  Expr* e = (Expr*)calloc(1, sizeof(Expr));
  return e;
}

static void expr_free(Expr* e) {
  if (!e) return;
  if (e->type == EXPR_LITERAL) value_free(&e->lit);
  expr_free(e->left);
  expr_free(e->right);
  expr_free(e->call.arg);
  free(e);
}

void program_free(Program* p) {
  if (!p) return;
  for (size_t i = 0; i < p->count; i++) {
    expr_free(p->stmts[i].expr);
  }
  free(p->stmts);
  p->stmts = NULL;
  p->count = 0;
  p->cap = 0;
}

static void prog_push(Program* p, Stmt s) {
  if (p->count + 1 > p->cap) {
    size_t nc = p->cap ? p->cap * 2 : 16;
    p->stmts = (Stmt*)realloc(p->stmts, nc * sizeof(Stmt));
    p->cap = nc;
  }
  p->stmts[p->count++] = s;
}

typedef struct Parser {
  Lexer lx;
  Token cur;
  Token prev;
} Parser;

static void adv(Parser* ps) {
  ps->prev = ps->cur;
  ps->cur = lexer_next(&ps->lx);
}

static bool match(Parser* ps, TokenType t) {
  if (ps->cur.type == t) {
    adv(ps);
    return true;
  }
  return false;
}

static void consume(Parser* ps, TokenType t, ParseError* err, const char* msg) {
  if (err && err->has_error) return;
  if (ps->cur.type == t) { adv(ps); return; }
  set_error(err, ps->cur.line, ps->cur.col, msg);
}

static Expr* parse_expr(Parser* ps, ParseError* err);

static Expr* parse_primary(Parser* ps, ParseError* err) {
  if (err && err->has_error) return NULL;

  // literal
  if (ps->cur.type == TOK_STRING) {
    Expr* e = expr_new();
    e->type = EXPR_LITERAL;
    e->tok = ps->cur;
    e->lit = value_string(ps->cur.start, ps->cur.length);
    adv(ps);
    return e;
  }
  if (ps->cur.type == TOK_NUMBER) {
    Expr* e = expr_new();
    e->type = EXPR_LITERAL;
    e->tok = ps->cur;
    e->lit = value_int(ps->cur.number);
    adv(ps);
    return e;
  }

  // ask(...) call
  if (ps->cur.type == TOK_ASK) {
    Expr* e = expr_new();
    e->type = EXPR_CALL;
    e->call.callee = ps->cur;
    adv(ps);
    consume(ps, TOK_LPAREN, err, "expected '(' after ask");
    e->call.arg = parse_expr(ps, err);
    consume(ps, TOK_RPAREN, err, "expected ')' after ask argument");
    return e;
  }

  // identifier
  if (ps->cur.type == TOK_IDENT) {
    Expr* e = expr_new();
    e->type = EXPR_IDENT;
    e->tok = ps->cur;
    adv(ps);
    return e;
  }

  set_error(err, ps->cur.line, ps->cur.col, "expected expression");
  return NULL;
}

static Expr* parse_term(Parser* ps, ParseError* err) {
  return parse_primary(ps, err);
}

static Expr* parse_expr(Parser* ps, ParseError* err) {
  Expr* left = parse_term(ps, err);
  if (err && err->has_error) return left;

  while (ps->cur.type == TOK_PLUS) {
    Token op = ps->cur;
    adv(ps);
    Expr* right = parse_term(ps, err);
    Expr* bin = expr_new();
    bin->type = EXPR_BINARY;
    bin->tok = op;
    bin->op = BIN_ADD;
    bin->left = left;
    bin->right = right;
    left = bin;
  }
  return left;
}

static void skip_newlines(Parser* ps) {
  while (ps->cur.type == TOK_NEWLINE) adv(ps);
}

static Stmt parse_stmt(Parser* ps, ParseError* err) {
  Stmt s;
  memset(&s, 0, sizeof(s));
  s.type = STMT_UNSUPPORTED;
  s.line = ps->cur.line;

  if (match(ps, TOK_SHOW) || match(ps, TOK_SAY)) {
    s.type = STMT_SHOW;
    s.expr = parse_expr(ps, err);
    return s;
  }
  if (match(ps, TOK_WARN)) {
    s.type = STMT_WARN;
    s.expr = parse_expr(ps, err);
    return s;
  }
  if (match(ps, TOK_SET) || match(ps, TOK_LOCK)) {
    bool is_lock = (ps->prev.type == TOK_LOCK);
    s.type = is_lock ? STMT_LOCK : STMT_SET;
    if (ps->cur.type != TOK_IDENT) {
      set_error(err, ps->cur.line, ps->cur.col, "expected identifier after set/lock");
      return s;
    }
    s.name = ps->cur;
    adv(ps);
    consume(ps, TOK_TO, err, "expected 'to' after identifier");
    s.expr = parse_expr(ps, err);
    return s;
  }

  // reserved keywords -> explicit error for now
  if (ps->cur.type == TOK_DEFINE || ps->cur.type == TOK_IF || ps->cur.type == TOK_LOOP ||
      ps->cur.type == TOK_REPEAT || ps->cur.type == TOK_TRY || ps->cur.type == TOK_MODULE ||
      ps->cur.type == TOK_START) {
    set_error(err, ps->cur.line, ps->cur.col, "statement reserved but not implemented in seed0");
    return s;
  }

  set_error(err, ps->cur.line, ps->cur.col, "unknown statement");
  return s;
}

Program parse_source(const char* src, size_t len, ParseError* err) {
  Program p;
  memset(&p, 0, sizeof(p));
  if (err) memset(err, 0, sizeof(*err));

  Parser ps;
  lexer_init(&ps.lx, src, len, 1);
  ps.cur = lexer_next(&ps.lx);
  ps.prev = ps.cur;

  skip_newlines(&ps);

  while (ps.cur.type != TOK_EOF) {
    if (err && err->has_error) break;
    Stmt s = parse_stmt(&ps, err);
    prog_push(&p, s);

    // consume rest of line until newline or EOF
    while (ps.cur.type != TOK_NEWLINE && ps.cur.type != TOK_EOF) {
      // if we get here, the statement parser didn't consume all tokens -> error
      set_error(err, ps.cur.line, ps.cur.col, "unexpected token at end of statement");
      break;
    }
    if (ps.cur.type == TOK_NEWLINE) adv(&ps);
    skip_newlines(&ps);
  }

  if (err && err->has_error) {
    // free expressions to avoid leaks
    program_free(&p);
    memset(&p, 0, sizeof(p));
  }
  return p;
}
