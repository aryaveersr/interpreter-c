#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
  const char* start;
  const char* next;
  int line;
  Token buffer;
} Lexer;

static Lexer lexer;

#define IS_VALID_IN_IDENTIFIER(ch) (isalnum(ch) || (ch) == '_')
#define BUFFER_IS_EMPTY() (lexer.buffer.len == -1)
#define BUFFER_CLEAR() (lexer.buffer.len = -1)

static Token emit_token(TokenKind kind) {
  Token token;

  token.kind = kind;
  token.line = lexer.line;
  token.start = lexer.start;
  token.len = (int) (lexer.next - lexer.start);

  return token;
}

static Token emit_error(const char* message) {
  Token token;

  token.kind = TOKEN_ERROR;
  token.line = lexer.line;
  token.start = message;
  token.len = (int) strlen(message);

  return token;
}

static bool is_eof(void) {
  return *lexer.next == '\0';
}

static char advance(void) {
  lexer.next += 1;
  return lexer.next[-1];
}

static bool match(char ch) {
  if (is_eof() || *lexer.next != ch) {
    return false;
  }

  lexer.next += 1;
  return true;
}

static void skip_whitespace(void) {
  while (true) {
    char ch = *lexer.next;

    switch (ch) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;

      case '\n':
        lexer.line += 1;
        advance();
        break;

      case '/':
        if (lexer.next[1] == '/') {
          while (*lexer.next != '\n' && !is_eof()) {
            advance();
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

static Token number(void) {
  while (isdigit(*lexer.next)) {
    lexer.next += 1;
  }

  if (*lexer.next == '.' && isdigit(lexer.next[1])) {
    lexer.next += 1;

    while (isdigit(*lexer.next)) {
      lexer.next += 1;
    }
  }

  return emit_token(TOKEN_NUMBER);
}

static Token string(void) {
  while (*lexer.next != '"' && !is_eof()) {
    if (advance() == '\n') {
      lexer.line += 1;
    }
  }

  if (match('"')) {
    return emit_token(TOKEN_STRING);
  } else {
    return emit_error("Unterminated string.");
  }
}

static TokenKind identifier_kind(void) {
#define X(kind, keyword)                                         \
  if ((lexer.next - lexer.start) == (sizeof(keyword) - 1) &&     \
      strncmp(lexer.start, keyword, sizeof(keyword) - 1) == 0) { \
    return kind;                                                 \
  }

  KEYWORD_TOKENS(X)
#undef X

  return TOKEN_IDENTIFIER;
}

static Token identifier(void) {
  while (IS_VALID_IN_IDENTIFIER(*lexer.next)) {
    lexer.next += 1;
  }

  return emit_token(identifier_kind());
}

void lexer_init(const char* source) {
  lexer.start = source;
  lexer.next = source;
  lexer.line = 1;

  BUFFER_CLEAR();
}

Token lexer_next(void) {
  if (!BUFFER_IS_EMPTY()) {
    Token buffer = lexer.buffer;
    BUFFER_CLEAR();
    return buffer;
  }

  skip_whitespace();
  lexer.start = lexer.next;

  if (is_eof()) {
    return emit_token(TOKEN_EOF);
  }

  char ch = advance();

  if (isdigit(ch)) {
    return number();
  }

  if (IS_VALID_IN_IDENTIFIER(ch)) {
    return identifier();
  }

  switch (ch) {
#define X(x, y) \
  case y:       \
    return emit_token(x);

    SINGLE_CHAR_TOKENS(X)
#undef X

#define X(x, y, z) \
  case z:          \
    return emit_token(match('=') ? (y) : (x));

    MAYBE_EQ_TOKENS(X)
#undef X

    case '"':
      return string();

    default:
      return emit_error("Unknown character.");
  }
}

Token lexer_peek(void) {
  lexer.buffer = lexer_next();
  return lexer.buffer;
}
