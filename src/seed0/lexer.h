#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef enum TokenType {
  TOK_EOF = 0,
  TOK_NEWLINE,

  TOK_IDENT,
  TOK_STRING,
  TOK_NUMBER,

  TOK_LPAREN,
  TOK_RPAREN,
  TOK_COMMA,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,
  TOK_EQUAL_EQUAL,
  TOK_BANG_EQUAL,
  TOK_LT,
  TOK_LTE,
  TOK_GT,
  TOK_GTE,
  TOK_COLON,

  // keywords
  TOK_SET,
  TOK_LOCK,
  TOK_TO,
  TOK_SHOW,
  TOK_SAY,
  TOK_WARN,
  TOK_ASK,

  // reserved (parsed as keywords but may be unimplemented in seed0)
  TOK_DEFINE,
  TOK_IF,
  TOK_THEN,
  TOK_OTHERWISE,
  TOK_LOOP,
  TOK_FOREVER,
  TOK_REPEAT,
  TOK_FROM,
  TOK_TRY,
  TOK_ON,
  TOK_ERROR,
  TOK_MODULE,
  TOK_START,
  TOK_WITH,
  TOK_AS,
  TOK_AND,
  TOK_OR,
  TOK_NOT,
  TOK_ARROW,
  TOK_RETURN,
  TOK_BREAK,
  TOK_CONTINUE
} TokenType;

typedef struct Token {
  TokenType type;
  const char* start;   // pointer into source line buffer
  size_t length;
  long number;         // if TOK_NUMBER
  size_t line;
  size_t col;
} Token;

typedef struct Lexer {
  const char* src;
  size_t len;
  size_t pos;
  size_t line;
  size_t col;
} Lexer;

void lexer_init(Lexer* lx, const char* src, size_t len, size_t line);
Token lexer_next(Lexer* lx);
const char* token_type_name(TokenType t);
