#ifndef LEXER_H
#define LEXER_H

#define SINGLE_CHAR_TOKENS(_) \
  /* */                       \
  /* Single-characters. */    \
  /* */                       \
  _(TOKEN_LEFT_PAREN, '(')    \
  _(TOKEN_RIGHT_PAREN, ')')   \
  _(TOKEN_LEFT_BRACE, '{')    \
  _(TOKEN_RIGHT_BRACE, '}')   \
  _(TOKEN_COMMA, ',')         \
  _(TOKEN_DOT, '.')           \
  _(TOKEN_COLON, ':')         \
  _(TOKEN_SEMICOLON, ';')     \
  _(TOKEN_PLUS, '+')          \
  _(TOKEN_MINUS, '-')         \
  _(TOKEN_STAR, '*')          \
  _(TOKEN_SLASH, '/')

#define KEYWORD_TOKENS(_)   \
  /* */                     \
  /* Keywords. */           \
  /* */                     \
  _(TOKEN_FUN, "fun")       \
  _(TOKEN_RETURN, "return") \
  _(TOKEN_IF, "if")         \
  _(TOKEN_ELSE, "else")     \
  _(TOKEN_FOR, "for")       \
  _(TOKEN_WHILE, "while")   \
  _(TOKEN_LET, "let")       \
  _(TOKEN_CLASS, "class")   \
  _(TOKEN_SUPER, "super")   \
  _(TOKEN_SELF, "self")     \
  _(TOKEN_PRINT, "print")   \
  _(TOKEN_AND, "and")       \
  _(TOKEN_OR, "or")

#define MAYBE_EQ_TOKENS(_)                 \
  /* */                                    \
  /* Tokens optionally ending with '='. */ \
  /* */                                    \
  _(TOKEN_BANG, TOKEN_BANG_EQUAL, '!')     \
  _(TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, '=')   \
  _(TOKEN_LESSER, TOKEN_LESSER_EQUAL, '<') \
  _(TOKEN_GREATER, TOKEN_GREATER_EQUAL, '>')

#define MISC_TOKENS(_)               \
  /* */                              \
  /* Literals and special tokens. */ \
  /* */                              \
  _(TOKEN_IDENTIFIER)                \
  _(TOKEN_STRING)                    \
  _(TOKEN_NUMBER)                    \
  _(TOKEN_NIL)                       \
  _(TOKEN_TRUE)                      \
  _(TOKEN_FALSE)                     \
  _(TOKEN_EOF)                       \
  _(TOKEN_ERROR)

typedef enum {

#define X(x) x,
  MISC_TOKENS(X) //
#undef X

#define X(x, y) x,
  SINGLE_CHAR_TOKENS(X) //
  KEYWORD_TOKENS(X)     //
#undef X

#define X(x, y, z) x, y,
  MAYBE_EQ_TOKENS(X)
#undef X

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
