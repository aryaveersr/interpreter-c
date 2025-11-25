#ifndef LEXER_H
#define LEXER_H

typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,

  // One or two character tokens.
  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESSER,
  TOKEN_LESSER_EQUAL,

  // Literals.
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,

  TOKEN_NIL,
  TOKEN_TRUE,
  TOKEN_FALSE,

  // Keywords.
  TOKEN_FUN,
  TOKEN_RETURN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_FOR,
  TOKEN_WHILE,
  TOKEN_LET,
  TOKEN_CLASS,
  TOKEN_SUPER,
  TOKEN_SELF,
  TOKEN_PRINT,
  TOKEN_AND,
  TOKEN_OR,

  TOKEN_EOF,
  TOKEN_ERROR,
} TokenKind;

typedef struct {
  TokenKind kind;
  const char* start;
  int len;
  int line;
} Token;

void lexer_init(const char* source);

Token lexer_next(void);
Token lexer_peek(void);

#endif
