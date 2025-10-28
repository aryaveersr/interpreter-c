#include "lexer.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static struct {
  const char *start;
  const char *next;
  int line;
  Token buffer;
} lexer;

#define IS_VALID_IN_IDENTIFIER(ch) (isalnum(ch) || (ch) == '_')

#define BUFFER_IS_EMPTY() (lexer.buffer.len == -1)
#define BUFFER_CLEAR() (lexer.buffer.len = -1)

void lexer_init(const char *source) {
  lexer.start = source;
  lexer.next = source;
  lexer.line = 1;

  BUFFER_CLEAR();
}

static char lx_advance(void) {
  lexer.next += 1;
  return lexer.next[-1];
}

static bool lx_is_eof(void) {
  return *lexer.next == '\0';
}

static void lx_skip_whitespace(void) {
  while (true) {
    char ch = *lexer.next;

    switch (ch) {
    case ' ':
    case '\r':
    case '\t':
      lx_advance();
      break;

    case '\n':
      lexer.line += 1;
      lx_advance();
      break;

    case '/':
      if (lexer.next[1] == '/') {
        while (*lexer.next != '\n' && !lx_is_eof()) {
          lx_advance();
        }
      } else {
        return;
      }

      break;

    default:
      return;
    }
  }
}

static Token lx_emit_token(TokenKind kind) {
  Token token;

  token.kind = kind;
  token.line = lexer.line;
  token.start = lexer.start;
  token.len = (int)(lexer.next - lexer.start);

  return token;
}

static Token lx_emit_error(const char *message) {
  Token token;

  token.kind = TOKEN_ERROR;
  token.line = lexer.line;
  token.start = message;
  token.len = (int)strlen(message);

  return token;
}

static bool lx_match(char ch) {
  if (lx_is_eof() || *lexer.next != ch) {
    return false;
  }

  lexer.next += 1;
  return true;
}

static Token lx_number(void) {
  while (isdigit(*lexer.next)) {
    lexer.next += 1;
  }

  if (*lexer.next == '.' && isdigit(lexer.next[1])) {
    lexer.next += 1;

    while (isdigit(*lexer.next)) {
      lexer.next += 1;
    }
  }

  return lx_emit_token(TOKEN_NUMBER);
}

static Token lx_string(void) {
  while (*lexer.next != '"' && !lx_is_eof()) {
    if (lx_advance() == '\n') {
      lexer.line += 1;
    }
  }

  if (lx_match('"')) {
    return lx_emit_token(TOKEN_STRING);
  } else {
    return lx_emit_error("Unterminated string.");
  }
}

static TokenKind lx_identifier_kind(void) {
#define MATCH_KEYWORD(keyword, kind)                                           \
  if ((lexer.next - lexer.start) == (sizeof(keyword) - 1) &&                   \
      strncmp(lexer.start, keyword, sizeof(keyword) - 1) == 0) {               \
    return kind;                                                               \
  }

  // clang-format off
  MATCH_KEYWORD("function", TOKEN_FUNCTION)
  else MATCH_KEYWORD("return", TOKEN_RETURN)
  else MATCH_KEYWORD("if", TOKEN_IF)
  else MATCH_KEYWORD("else", TOKEN_ELSE)
  else MATCH_KEYWORD("for", TOKEN_FOR)
  else MATCH_KEYWORD("while", TOKEN_WHILE)
  else MATCH_KEYWORD("let", TOKEN_LET)
  else MATCH_KEYWORD("class", TOKEN_CLASS)
  else MATCH_KEYWORD("super", TOKEN_SUPER)
  else MATCH_KEYWORD("self", TOKEN_SELF)
  else MATCH_KEYWORD("print", TOKEN_PRINT)
  else MATCH_KEYWORD("and", TOKEN_AND)
  else MATCH_KEYWORD("or", TOKEN_OR)
  else MATCH_KEYWORD("nil", TOKEN_NIL)
  else MATCH_KEYWORD("true", TOKEN_TRUE)
  else MATCH_KEYWORD("false", TOKEN_FALSE);
  // clang-format on

  return TOKEN_IDENTIFIER;
#undef MATCH_KEYWORD
}

static Token lx_identifier(void) {
  while (IS_VALID_IN_IDENTIFIER(*lexer.next)) {
    lexer.next += 1;
  }

  return lx_emit_token(lx_identifier_kind());
}

Token lexer_next(void) {
  if (!BUFFER_IS_EMPTY()) {
    Token buffer = lexer.buffer;
    BUFFER_CLEAR();
    return buffer;
  }

  lx_skip_whitespace();
  lexer.start = lexer.next;

  if (lx_is_eof()) {
    return lx_emit_token(TOKEN_EOF);
  }

  char ch = lx_advance();

  if (isdigit(ch)) {
    return lx_number();
  }

  if (IS_VALID_IN_IDENTIFIER(ch)) {
    return lx_identifier();
  }

#define SINGLE_CHAR_TOKEN(ch, kind)                                            \
  case ch:                                                                     \
    return lx_emit_token(kind)

#define EQUAL_TOKEN(ch, without_eq, with_eq)                                   \
  case ch:                                                                     \
    return lx_emit_token(lx_match('=') ? (with_eq) : (without_eq))

  switch (ch) {
    SINGLE_CHAR_TOKEN('(', TOKEN_LEFT_PAREN);
    SINGLE_CHAR_TOKEN(')', TOKEN_RIGHT_PAREN);
    SINGLE_CHAR_TOKEN('{', TOKEN_LEFT_BRACE);
    SINGLE_CHAR_TOKEN('}', TOKEN_RIGHT_BRACE);
    SINGLE_CHAR_TOKEN(':', TOKEN_COLON);
    SINGLE_CHAR_TOKEN(';', TOKEN_SEMICOLON);
    SINGLE_CHAR_TOKEN(',', TOKEN_COMMA);
    SINGLE_CHAR_TOKEN('.', TOKEN_DOT);
    SINGLE_CHAR_TOKEN('+', TOKEN_PLUS);
    SINGLE_CHAR_TOKEN('-', TOKEN_MINUS);
    SINGLE_CHAR_TOKEN('*', TOKEN_STAR);
    SINGLE_CHAR_TOKEN('/', TOKEN_SLASH);

    EQUAL_TOKEN('!', TOKEN_BANG, TOKEN_BANG_EQUAL);
    EQUAL_TOKEN('=', TOKEN_EQUAL, TOKEN_EQUAL_EQUAL);
    EQUAL_TOKEN('<', TOKEN_LESSER, TOKEN_LESSER_EQUAL);
    EQUAL_TOKEN('>', TOKEN_GREATER, TOKEN_GREATER_EQUAL);

  case '"':
    return lx_string();

  default:
    return lx_emit_error("Unknown character.");
  }

#undef SINGLE_CHAR_TOKEN
#undef EQUAL_TOKEN
}

Token lexer_peek(void) {
  if (BUFFER_IS_EMPTY()) {
    lexer.buffer = lexer_next();
  }

  return lexer.buffer;
}
