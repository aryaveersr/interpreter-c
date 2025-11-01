#include "compiler.h"
#include "chunk.h"
#include "lexer.h"
#include "object.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  bool had_error;
  bool panic;
  Token last;
} Parser;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_MAX + 1];
  int len;
  int depth;
} Scope;

typedef struct {
  Chunk *current_chunk;
  Scope *scope;
} Compiler;

static Parser parser;
static Compiler compiler;

static void c_report_error(Token *token, const char *message) {
  if (parser.panic) {
    return;
  }

  parser.panic = true;
  parser.had_error = true;

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
      parser.last = next;
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
  chunk_write(compiler.current_chunk, byte, parser.last.line);
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

static int c_resolve_local(Scope *scope, Token *name) {
  for (int i = scope->len - 1; i >= 0; i--) {
    Local *local = &scope->locals[i];
    if (local->name.len == name->len &&
        memcmp(local->name.start, name->start, name->len) == 0) {
      if (local->depth == -1) {
        c_report_error(&parser.last,
                       "Can't read variable in it's own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static void c_named_variable(Token *token) {
  uint8_t set = OP_SET_GLOBAL;
  uint8_t get = OP_GET_GLOBAL;
  int idx = c_resolve_local(compiler.scope, token);

  if (idx == -1) {
    idx = c_identifier_const(token);
  } else {
    set = OP_SET_LOCAL;
    get = OP_GET_LOCAL;
  }

  if (c_match(TOKEN_EQUAL)) {
    c_expression();
    c_emit_byte(set);
    c_emit_byte((uint8_t)idx);
  } else {
    c_emit_byte(get);
    c_emit_byte((uint8_t)idx);
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

static void scope_begin(void) {
  compiler.scope->depth += 1;
}

static void scope_end(void) {
  compiler.scope->depth -= 1;

  while (compiler.scope->len > 0 &&
         compiler.scope->locals[compiler.scope->len - 1].depth >
             compiler.scope->depth) {
    c_emit_byte(OP_POP);
    compiler.scope->len--;
  }
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

static void stmt_block(void) {
  while (!c_match(TOKEN_RIGHT_BRACE) && !c_match(TOKEN_EOF)) {
    c_declaration();
  }

  if (parser.last.kind != TOKEN_RIGHT_BRACE) {
    c_report_error(&parser.last, "Expected } after block.");
  }
}

static void c_statement(void) {
  if (c_match(TOKEN_PRINT)) {
    stmt_print();
  } else if (c_match(TOKEN_LEFT_BRACE)) {
    scope_begin();
    stmt_block();
    scope_end();
  } else {
    stmt_expression();
  }
}

static void c_synchronize(void) {
  parser.panic = false;

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

static void c_add_local(Token name) {
  if (compiler.scope->len > UINT8_MAX) {
    c_report_error(&parser.last, "Too many local variables in the function.");
    return;
  }

  Local *local = &compiler.scope->locals[compiler.scope->len++];

  local->name = name;
  local->depth = -1;
}

static void c_declare_variable(void) {
  if (compiler.scope->depth == 0) {
    return;
  }

  for (int i = compiler.scope->len - 1; i >= 0; i--) {
    Local *local = &compiler.scope->locals[i];

    if (local->depth != -1 && local->depth < compiler.scope->depth) {
      break;
    }

    if (parser.last.len == local->name.len &&
        memcmp(parser.last.start, local->name.start, parser.last.len) == 0) {
      c_report_error(&parser.last,
                     "A variable with this name already exists in the scope.");
    }
  }

  c_add_local(parser.last);
}

static uint8_t c_variable(const char *message) {
  c_expect(TOKEN_IDENTIFIER, message);
  c_declare_variable();

  if (compiler.scope->depth > 0) {
    return 0;
  }

  return c_identifier_const(&parser.last);
}

static void c_define_variable(uint8_t global) {
  if (compiler.scope->depth > 0) {
    compiler.scope->locals[compiler.scope->len - 1].depth =
        compiler.scope->depth;
    return;
  }

  c_emit_byte(OP_DEFINE_GLOBAL);
  c_emit_byte(global);
}

static void decl_variable(void) {
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
    decl_variable();
  } else {
    c_statement();
  }

  if (parser.panic) {
    c_synchronize();
  }
}

bool compiler_compile(Chunk *chunk) {
  Scope scope = {.depth = 0, .len = 0};

  parser.had_error = false;
  parser.panic = false;
  compiler.current_chunk = chunk;
  compiler.scope = &scope;

  while (!c_match(TOKEN_EOF)) {
    c_declaration();
  }

  c_emit_byte(OP_RETURN);

  return !parser.had_error;
}
