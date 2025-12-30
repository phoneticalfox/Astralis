#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void set_error(ParseError* err, size_t line, size_t col, const char* msg) {
  if (!err) return;
  if (err->has_error) return;
  err->has_error = true;
  err->line = line;
  err->col = col;
  snprintf(err->message, sizeof(err->message), "%s", msg);
}

static Expr* expr_new(void) {
  return (Expr*)calloc(1, sizeof(Expr));
}

static void expr_free(Expr* e) {
  if (!e) return;
  if (e->type == EXPR_LITERAL) value_free(&e->lit);
  expr_free(e->left);
  expr_free(e->right);
  if (e->type == EXPR_CALL) {
    expr_free(e->call.callee);
    for (size_t i = 0; i < e->call.arg_count; i++) expr_free(e->call.args[i]);
    free(e->call.args);
  }
  free(e);
}

static void block_free(Block* b) {
  if (!b) return;
  for (size_t i = 0; i < b->count; i++) {
    expr_free(b->stmts[i].expr);
    expr_free(b->stmts[i].expr_b);
    block_free(b->stmts[i].block);
    block_free(b->stmts[i].else_block);
    free(b->stmts[i].params);
  }
  free(b->stmts);
  b->stmts = NULL; b->count = 0; b->cap = 0;
}

void program_free(Program* p) {
  if (!p) return;
  block_free(&p->block);
}

static void block_push(Block* b, Stmt s) {
  if (b->count + 1 > b->cap) {
    size_t nc = b->cap ? b->cap * 2 : 16;
    b->stmts = (Stmt*)realloc(b->stmts, nc * sizeof(Stmt));
    b->cap = nc;
  }
  b->stmts[b->count++] = s;
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
  if (ps->cur.type == t) { adv(ps); return true; }
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
  if (ps->cur.type == TOK_IDENT || ps->cur.type == TOK_ASK) {
    Expr* e = expr_new();
    e->type = EXPR_IDENT;
    e->tok = ps->cur;
    adv(ps);
    return e;
  }
  if (ps->cur.type == TOK_LPAREN) {
    adv(ps);
    Expr* inner = parse_expr(ps, err);
    consume(ps, TOK_RPAREN, err, "expected ')' after group");
    Expr* e = expr_new();
    e->type = EXPR_GROUP;
    e->left = inner;
    return e;
  }

  set_error(err, ps->cur.line, ps->cur.col, "expected expression");
  return NULL;
}

static Expr* parse_call(Parser* ps, ParseError* err) {
  Expr* expr = parse_primary(ps, err);
  while (ps->cur.type == TOK_LPAREN) {
    adv(ps); // consume '('
    Expr** args = NULL;
    size_t argc = 0, cap = 0;
    if (ps->cur.type != TOK_RPAREN) {
      do {
        if (argc + 1 > cap) {
          size_t nc = cap ? cap * 2 : 4;
          args = (Expr**)realloc(args, nc * sizeof(Expr*));
          cap = nc;
        }
        args[argc++] = parse_expr(ps, err);
      } while (match(ps, TOK_COMMA));
    }
    consume(ps, TOK_RPAREN, err, "expected ')' after arguments");
    Expr* call = expr_new();
    call->type = EXPR_CALL;
    call->call.callee = expr;
    call->call.args = args;
    call->call.arg_count = argc;
    expr = call;
  }
  return expr;
}

static Expr* parse_term(Parser* ps, ParseError* err) {
  return parse_call(ps, err);
}

static Expr* parse_expr(Parser* ps, ParseError* err) {
  Expr* left = parse_term(ps, err);
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

static Block parse_block(Parser* ps, ParseError* err, size_t indent);
static Stmt parse_stmt(Parser* ps, ParseError* err, size_t indent);

static bool is_block_connector(TokenType t) {
  return t == TOK_ARROW || t == TOK_COLON || t == TOK_AS || t == TOK_THEN;
}

static Block* parse_inline_block(Parser* ps, ParseError* err, size_t indent) {
  (void)indent;
  Block* b = (Block*)calloc(1, sizeof(Block));
  Stmt s;
  memset(&s, 0, sizeof(s));
  s = (Stmt){0};
  s.type = STMT_UNSUPPORTED;
  s.line = ps->cur.line;

  // parse a single statement inline
  s = parse_stmt(ps, err, indent);
  block_push(b, s);
  return b;
}

static Stmt parse_stmt(Parser* ps, ParseError* err, size_t indent) {
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
  if (match(ps, TOK_RETURN)) {
    s.type = STMT_RETURN;
    if (ps->cur.type != TOK_NEWLINE && ps->cur.type != TOK_EOF)
      s.expr = parse_expr(ps, err);
    return s;
  }
  if (match(ps, TOK_BREAK)) { s.type = STMT_BREAK; return s; }
  if (match(ps, TOK_CONTINUE)) { s.type = STMT_CONTINUE; return s; }

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

  if (match(ps, TOK_IF)) {
    s.type = STMT_IF;
    s.expr = parse_expr(ps, err);
    if (is_block_connector(ps->cur.type)) adv(ps);
    if (ps->cur.type != TOK_NEWLINE && ps->cur.type != TOK_EOF) {
      s.block = parse_inline_block(ps, err, indent);
      return s;
    }
    consume(ps, TOK_NEWLINE, err, "expected newline after if condition");
    skip_newlines(ps);
    size_t body_indent = ps->cur.col;
    s.block = (Block*)calloc(1, sizeof(Block));
    *s.block = parse_block(ps, err, body_indent);
    if (ps->cur.type == TOK_OTHERWISE) {
      adv(ps);
      if (is_block_connector(ps->cur.type)) adv(ps);
      if (ps->cur.type != TOK_NEWLINE && ps->cur.type != TOK_EOF) {
        s.else_block = parse_inline_block(ps, err, indent);
        return s;
      }
      consume(ps, TOK_NEWLINE, err, "expected newline after otherwise");
      skip_newlines(ps);
      size_t else_indent = ps->cur.col;
      s.else_block = (Block*)calloc(1, sizeof(Block));
      *s.else_block = parse_block(ps, err, else_indent);
    }
    return s;
  }

  if (match(ps, TOK_LOOP)) {
    s.type = STMT_LOOP_FOREVER;
    if (match(ps, TOK_FOREVER)) {
      // ok
    }
    if (is_block_connector(ps->cur.type)) adv(ps);
    if (ps->cur.type != TOK_NEWLINE && ps->cur.type != TOK_EOF) {
      s.block = parse_inline_block(ps, err, indent);
      return s;
    }
    consume(ps, TOK_NEWLINE, err, "expected newline after loop forever");
    skip_newlines(ps);
    size_t body_indent = ps->cur.col;
    s.block = (Block*)calloc(1, sizeof(Block));
    *s.block = parse_block(ps, err, body_indent);
    return s;
  }

  if (match(ps, TOK_REPEAT)) {
    s.type = STMT_REPEAT;
    if (ps->cur.type != TOK_IDENT) {
      set_error(err, ps->cur.line, ps->cur.col, "expected identifier after repeat");
      return s;
    }
    s.loop_var = ps->cur;
    adv(ps);
    consume(ps, TOK_FROM, err, "expected 'from' after loop variable");
    s.expr = parse_expr(ps, err);
    consume(ps, TOK_TO, err, "expected 'to' after loop lower bound");
    s.expr_b = parse_expr(ps, err);
    if (is_block_connector(ps->cur.type)) adv(ps);
    if (ps->cur.type != TOK_NEWLINE && ps->cur.type != TOK_EOF) {
      s.block = parse_inline_block(ps, err, indent);
      return s;
    }
    consume(ps, TOK_NEWLINE, err, "expected newline after repeat header");
    skip_newlines(ps);
    size_t body_indent = ps->cur.col;
    s.block = (Block*)calloc(1, sizeof(Block));
    *s.block = parse_block(ps, err, body_indent);
    return s;
  }

  if (match(ps, TOK_DEFINE)) {
    s.type = STMT_DEFINE;
    if (ps->cur.type != TOK_IDENT) {
      set_error(err, ps->cur.line, ps->cur.col, "expected function name after define");
      return s;
    }
    s.name = ps->cur;
    adv(ps);
    consume(ps, TOK_LPAREN, err, "expected '(' after function name");
    Token* params = NULL; size_t pc = 0, pcap = 0;
    if (ps->cur.type != TOK_RPAREN) {
      do {
        if (ps->cur.type != TOK_IDENT) { set_error(err, ps->cur.line, ps->cur.col, "expected parameter name"); break; }
        if (pc + 1 > pcap) {
          size_t nc = pcap ? pcap * 2 : 4;
          params = (Token*)realloc(params, nc * sizeof(Token));
          pcap = nc;
        }
        params[pc++] = ps->cur;
        adv(ps);
      } while (match(ps, TOK_COMMA));
    }
    consume(ps, TOK_RPAREN, err, "expected ')' after parameters");
    s.params = params;
    s.param_count = pc;
    if (is_block_connector(ps->cur.type)) adv(ps);
    if (ps->cur.type != TOK_NEWLINE && ps->cur.type != TOK_EOF) {
      s.block = parse_inline_block(ps, err, indent);
      return s;
    }
    consume(ps, TOK_NEWLINE, err, "expected newline after function header");
    skip_newlines(ps);
    size_t body_indent = ps->cur.col;
    s.block = (Block*)calloc(1, sizeof(Block));
    *s.block = parse_block(ps, err, body_indent);
    return s;
  }

  // expression as statement (function calls etc.)
  if (ps->cur.type != TOK_EOF && ps->cur.type != TOK_NEWLINE) {
    s.type = STMT_EXPR;
    s.expr = parse_expr(ps, err);
    return s;
  }

  set_error(err, ps->cur.line, ps->cur.col, "unknown statement");
  return s;
}

static Block parse_block(Parser* ps, ParseError* err, size_t indent) {
  Block b; memset(&b, 0, sizeof(b));
  while (ps->cur.type != TOK_EOF) {
    if (ps->cur.col < indent) break;
    Stmt s = parse_stmt(ps, err, indent);
    block_push(&b, s);
    if (ps->cur.type == TOK_NEWLINE) {
      adv(ps);
    } else if (ps->cur.type != TOK_EOF && ps->cur.col > indent) {
      set_error(err, ps->cur.line, ps->cur.col, "unexpected token at end of statement");
      break;
    }
    skip_newlines(ps);
    if (err && err->has_error) break;
    if (ps->cur.col < indent) break;
  }
  if (err && err->has_error) {
    block_free(&b);
    memset(&b, 0, sizeof(b));
  }
  return b;
}

Program parse_source(const char* src, size_t len, ParseError* err) {
  Program p; memset(&p, 0, sizeof(p));
  if (err) memset(err, 0, sizeof(*err));

  Parser ps;
  lexer_init(&ps.lx, src, len, 1);
  ps.cur = lexer_next(&ps.lx);
  ps.prev = ps.cur;

  skip_newlines(&ps);
  p.block = parse_block(&ps, err, ps.cur.col ? ps.cur.col : 1);
  return p;
}
