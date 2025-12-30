#include "lexer.h"
#include <ctype.h>
#include <string.h>

static bool is_ident_start(char c) {
  return (c == '_') || isalpha((unsigned char)c);
}
static bool is_ident_char(char c) {
  return is_ident_start(c) || isdigit((unsigned char)c);
}

static Token make_token(Lexer* lx, TokenType type, const char* start, size_t length, long number, size_t col_start) {
  Token t;
  t.type = type;
  t.start = start;
  t.length = length;
  t.number = number;
  t.line = lx->line;
  t.col = col_start;
  return t;
}

void lexer_init(Lexer* lx, const char* src, size_t len, size_t line) {
  lx->src = src;
  lx->len = len;
  lx->pos = 0;
  lx->line = line;
  lx->col = 1;
}

static char peek(Lexer* lx) {
  if (lx->pos >= lx->len) return '\0';
  return lx->src[lx->pos];
}
static char peek2(Lexer* lx) {
  if (lx->pos + 1 >= lx->len) return '\0';
  return lx->src[lx->pos + 1];
}
static char advance(Lexer* lx) {
  if (lx->pos >= lx->len) return '\0';
  char c = lx->src[lx->pos++];
  if (c == '\n') {
    lx->line++;
    lx->col = 1;
  } else {
    lx->col++;
  }
  return c;
}

static void skip_ws_and_comments(Lexer* lx) {
  for (;;) {
    char c = peek(lx);
    // whitespace (not newline)
    while (c == ' ' || c == '\t' || c == '\r') {
      advance(lx);
      c = peek(lx);
    }
    // comment: consume until newline or EOF, but keep newline for the token stream
    if (c == '/' && peek2(lx) == '/') {
      while (peek(lx) != '\0' && peek(lx) != '\n') advance(lx);
      // do not consume '\n' here; lexer_next will produce TOK_NEWLINE
      return;
    }
    return;
  }
}

static TokenType keyword_type(const char* s, size_t n) {
  #define KW(lit, tok) if (n == sizeof(lit)-1 && memcmp(s, lit, sizeof(lit)-1) == 0) return tok
  KW("set", TOK_SET);
  KW("lock", TOK_LOCK);
  KW("to", TOK_TO);
  KW("show", TOK_SHOW);
  KW("say", TOK_SAY);
  KW("warn", TOK_WARN);
  KW("ask", TOK_ASK);

  // reserved / future
  KW("define", TOK_DEFINE);
  KW("if", TOK_IF);
  KW("then", TOK_THEN);
  KW("otherwise", TOK_OTHERWISE);
  KW("loop", TOK_LOOP);
  KW("forever", TOK_FOREVER);
  KW("repeat", TOK_REPEAT);
  KW("from", TOK_FROM);
  KW("try", TOK_TRY);
  KW("on", TOK_ON);
  KW("error", TOK_ERROR);
  KW("module", TOK_MODULE);
  KW("start", TOK_START);
  KW("with", TOK_WITH);
  KW("as", TOK_AS);
  KW("return", TOK_RETURN);
  KW("break", TOK_BREAK);
  KW("continue", TOK_CONTINUE);
  #undef KW
  return TOK_IDENT;
}

Token lexer_next(Lexer* lx) {
  skip_ws_and_comments(lx);

  const size_t col_start = lx->col;
  const char c = peek(lx);

  if (c == '\0') {
    return make_token(lx, TOK_EOF, lx->src + lx->pos, 0, 0, col_start);
  }

  // newline
  if (c == '\n') {
    advance(lx);
    return make_token(lx, TOK_NEWLINE, "\n", 1, 0, col_start);
  }

  // single-char tokens
  if (c == '(') { advance(lx); return make_token(lx, TOK_LPAREN, lx->src + lx->pos - 1, 1, 0, col_start); }
  if (c == ')') { advance(lx); return make_token(lx, TOK_RPAREN, lx->src + lx->pos - 1, 1, 0, col_start); }
  if (c == ',') { advance(lx); return make_token(lx, TOK_COMMA,  lx->src + lx->pos - 1, 1, 0, col_start); }
  if (c == '+') { advance(lx); return make_token(lx, TOK_PLUS,   lx->src + lx->pos - 1, 1, 0, col_start); }
  if (c == ':') { advance(lx); return make_token(lx, TOK_COLON,  lx->src + lx->pos - 1, 1, 0, col_start); }

  // arrow token "->"
  if (c == '-' && peek2(lx) == '>') {
    advance(lx); advance(lx);
    return make_token(lx, TOK_ARROW, lx->src + lx->pos - 2, 2, 0, col_start);
  }

  // string
  if (c == '"') {
    advance(lx); // opening quote
    const char* start = lx->src + lx->pos;
    size_t len = 0;
    while (peek(lx) != '\0' && peek(lx) != '"' && peek(lx) != '\n') {
      advance(lx);
      len++;
    }
    if (peek(lx) == '"') {
      advance(lx); // closing quote
      return make_token(lx, TOK_STRING, start, len, 0, col_start);
    }
    return make_token(lx, TOK_STRING, start, len, 0, col_start);
  }

  // number
  if (isdigit((unsigned char)c)) {
    const char* start = lx->src + lx->pos;
    size_t n = 0;
    long value = 0;
    while (isdigit((unsigned char)peek(lx))) {
      value = value * 10 + (advance(lx) - '0');
      n++;
    }
    return make_token(lx, TOK_NUMBER, start, n, value, col_start);
  }

  // identifier / keyword
  if (is_ident_start(c)) {
    const char* start = lx->src + lx->pos;
    size_t n = 0;
    while (is_ident_char(peek(lx))) {
      advance(lx);
      n++;
    }
    TokenType kt = keyword_type(start, n);
    return make_token(lx, kt, start, n, 0, col_start);
  }

  // unknown
  advance(lx);
  return make_token(lx, TOK_IDENT, lx->src + lx->pos - 1, 1, 0, col_start);
}

const char* token_type_name(TokenType t) {
  switch (t) {
    case TOK_EOF: return "EOF";
    case TOK_NEWLINE: return "NEWLINE";
    case TOK_IDENT: return "IDENT";
    case TOK_STRING: return "STRING";
    case TOK_NUMBER: return "NUMBER";
    case TOK_LPAREN: return "(";
    case TOK_RPAREN: return ")";
    case TOK_COMMA: return ",";
    case TOK_PLUS: return "+";
    case TOK_COLON: return ":";
    case TOK_SET: return "set";
    case TOK_LOCK: return "lock";
    case TOK_TO: return "to";
    case TOK_SHOW: return "show";
    case TOK_SAY: return "say";
    case TOK_WARN: return "warn";
    case TOK_ASK: return "ask";
    case TOK_DEFINE: return "define";
    case TOK_IF: return "if";
    case TOK_THEN: return "then";
    case TOK_OTHERWISE: return "otherwise";
    case TOK_LOOP: return "loop";
    case TOK_FOREVER: return "forever";
    case TOK_REPEAT: return "repeat";
    case TOK_FROM: return "from";
    case TOK_TRY: return "try";
    case TOK_ON: return "on";
    case TOK_ERROR: return "error";
    case TOK_MODULE: return "module";
    case TOK_START: return "start";
    case TOK_WITH: return "with";
    case TOK_AS: return "as";
    case TOK_ARROW: return "->";
    case TOK_RETURN: return "return";
    case TOK_BREAK: return "break";
    case TOK_CONTINUE: return "continue";
    default: return "<?>"; 
  }
}
