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

static bool c_match(TokenKind kind) {
  if (c_peek().kind != kind) {
    return false;
  }

  c_advance();
  return true;
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
static void c_statement(void);
static void c_declaration(void);

static uint8_t c_identifier_const(Token *token) {
  ObjString *name = string_copy(token->start, token->len);
  return (uint8_t)chunk_push_const(compiler.current_chunk, OBJ_VAL(name));
}

static void c_named_variable(Token *token) {
  uint8_t idx = c_identifier_const(token);

  if (c_match(TOKEN_EQUAL)) {
    c_expression();
    c_emit_byte(OP_SET_GLOBAL);
    c_emit_byte(idx);
  } else {
    c_emit_byte(OP_GET_GLOBAL);
    c_emit_byte(idx);
  }
}

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

  case TOKEN_IDENTIFIER:
    c_named_variable(&next);
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

static void stmt_print(void) {
  c_expression();
  c_expect(TOKEN_SEMICOLON, "Expected ; after value.");
  c_emit_byte(OP_PRINT);
}

static void stmt_expression(void) {
  c_expression();
  c_expect(TOKEN_SEMICOLON, "Expected ; after value.");
  c_emit_byte(OP_POP);
}

static void c_statement(void) {
  if (c_match(TOKEN_PRINT)) {
    stmt_print();
  } else {
    stmt_expression();
  }
}

static void c_synchronize(void) {
  compiler.panic = false;

  while (!c_match(TOKEN_EOF)) {
    switch (c_peek().kind) {
    case TOKEN_CLASS:
    case TOKEN_FUNCTION:
    case TOKEN_LET:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;

    case TOKEN_SEMICOLON:
      c_advance();
      return;

    default:
      c_advance();
      continue;
    }
  }
}

static uint8_t c_variable(const char *message) {
  c_expect(TOKEN_IDENTIFIER, message);
  return c_identifier_const(&compiler.last);
}

static void c_define_variable(uint8_t global) {
  c_emit_byte(OP_DEFINE_GLOBAL);
  c_emit_byte(global);
}

static void c_variable_decl(void) {
  uint8_t global = c_variable("Expected variable name.");

  if (c_match(TOKEN_EQUAL)) {
    c_expression();
  } else {
    c_emit_byte(OP_NIL);
  }

  c_expect(TOKEN_SEMICOLON, "Expected ; after variable declaration.");
  c_define_variable(global);
}

static void c_declaration(void) {
  if (c_match(TOKEN_LET)) {
    c_variable_decl();
  } else {
    c_statement();
  }

  if (compiler.panic) {
    c_synchronize();
  }
}

bool compiler_compile(Chunk *chunk) {
  compiler.had_error = false;
  compiler.panic = false;
  compiler.current_chunk = chunk;

  while (!c_match(TOKEN_EOF)) {
    c_declaration();
  }

  c_emit_byte(OP_RETURN);

  return !compiler.had_error;
}
