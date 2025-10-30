#include "compiler.h"
#include "chunk.h"
#include "lexer.h"
#include "object.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static struct {
  bool had_error;
  bool panic;
  Chunk *current_chunk;
  Token last;
} compiler;

static void c_report_error(Token *token, const char *message) {
  if (compiler.panic) {
    return;
  }

  compiler.panic = true;
  compiler.had_error = true;

  fprintf(stderr, "[Line:%d] Error", token->line);

  switch (token->kind) {
  case TOKEN_ERROR:
    break;

  case TOKEN_EOF:
    fprintf(stderr, " at end");
    break;

  default:
    fprintf(stderr, " at %.*s", token->len, token->start);
    break;
  }

  fprintf(stderr, ": %s\n", message);
}

static Token c_advance(void) {
  while (true) {
    Token next = lexer_next();

    if (next.kind != TOKEN_ERROR) {
      compiler.last = next;
      return next;
    }

    c_report_error(&next, next.start);
  }
}

static Token c_peek(void) {
  Token next = lexer_peek();

  while (next.kind == TOKEN_ERROR) {
    lexer_next();
    next = lexer_peek();
  }

  return next;
}

static void c_expect(TokenKind kind, const char *message) {
  Token next = c_peek();

  if (next.kind == kind) {
    c_advance();
  } else {
    c_report_error(&next, message);
  }
}

static void c_emit_byte(uint8_t byte) {
  chunk_write(compiler.current_chunk, byte, compiler.last.line);
}

static void c_emit_const(Value value) {
  int idx = chunk_push_const(compiler.current_chunk, value);

  c_emit_byte(OP_LOAD);
  c_emit_byte((uint8_t)idx);
}

// Forward declaration.
static void c_expression(void);

static void c_expr_primary(void) {
  Token next = c_advance();

  switch (next.kind) {
  case TOKEN_LEFT_PAREN:
    c_expression();
    c_expect(TOKEN_RIGHT_PAREN, "Expected closing parenthesis.");
    break;

  case TOKEN_NIL:
    c_emit_byte(OP_NIL);
    break;

  case TOKEN_TRUE:
    c_emit_byte(OP_TRUE);
    break;

  case TOKEN_FALSE:
    c_emit_byte(OP_FALSE);
    break;

  case TOKEN_NUMBER:
    c_emit_const(NUMBER_VAL(strtod(next.start, NULL)));
    break;

  case TOKEN_STRING:
    c_emit_const(OBJ_VAL(string_copy(next.start + 1, next.len - 2)));
    break;

  default:
    c_report_error(&next, "Expected expression.");
    break;
  }
}

static void c_expr_unary(void) {
  switch (c_peek().kind) {
  case TOKEN_MINUS:
    c_advance();
    c_expr_primary();
    c_emit_byte(OP_NEGATE);
    break;

  case TOKEN_BANG:
    c_advance();
    c_expr_primary();
    c_emit_byte(OP_NOT);
    break;

  default:
    c_expr_primary();
  }
}

static void c_expr_factor(void) {
  c_expr_unary();

  while ((c_peek().kind == TOKEN_STAR) || (c_peek().kind == TOKEN_SLASH)) {
    Token next = c_advance();

    c_expr_unary();
    c_emit_byte((next.kind == TOKEN_STAR) ? OP_MULTIPLY : OP_DIVIDE);
  }
}

static void c_expr_term(void) {
  c_expr_factor();

  while ((c_peek().kind == TOKEN_PLUS) || (c_peek().kind == TOKEN_MINUS)) {
    Token next = c_advance();

    c_expr_factor();
    c_emit_byte((next.kind == TOKEN_PLUS) ? OP_ADD : OP_SUBTRACT);
  }
}

static void c_expr_comparison(void) {
  c_expr_term();

  while (true) {
    switch (c_peek().kind) {
    case TOKEN_LESSER:
      c_advance();
      c_expr_term();
      c_emit_byte(OP_LESSER);
      continue;

    case TOKEN_LESSER_EQUAL:
      c_advance();
      c_expr_term();
      c_emit_byte(OP_GREATER);
      c_emit_byte(OP_NOT);
      continue;

    case TOKEN_GREATER:
      c_advance();
      c_expr_term();
      c_emit_byte(OP_GREATER);
      continue;

    case TOKEN_GREATER_EQUAL:
      c_advance();
      c_expr_term();
      c_emit_byte(OP_LESSER);
      c_emit_byte(OP_NOT);
      continue;

    case TOKEN_EQUAL_EQUAL:
      c_advance();
      c_expr_term();
      c_emit_byte(OP_EQUAL);
      continue;

    case TOKEN_BANG_EQUAL:
      c_advance();
      c_expr_term();
      c_emit_byte(OP_EQUAL);
      c_emit_byte(OP_NOT);
      continue;

    default:
      goto LOOP_END;
    }
  }

LOOP_END:
  (void)0;
}

static void c_expression(void) {
  c_expr_comparison();
}

bool compiler_compile(Chunk *chunk) {
  compiler.had_error = false;
  compiler.panic = false;
  compiler.current_chunk = chunk;

  c_expression();
  c_emit_byte(OP_RETURN);

  return !compiler.had_error;
}
