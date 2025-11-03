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
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_MAX + 1];
  int len;
  int depth;
} Compiler;

typedef struct {
  bool had_error;
  bool panic;
  Token last;
} Parser;

static Parser parser;
static Chunk *current_chunk;
static Compiler *current;

static void report_error_at(Token *token, const char *message) {
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

static inline void report_error(const char *message) {
  report_error_at(&parser.last, message);
}

static Token advance(void) {
  while (true) {
    Token next = lexer_next();
    parser.last = next;

    if (next.kind == TOKEN_ERROR) {
      report_error(next.start);
    } else {
      return next;
    }
  }
}

static Token peek(void) {
  Token next = lexer_peek();

  while (next.kind == TOKEN_ERROR) {
    lexer_next();
    next = lexer_peek();
  }

  return next;
}

static void expect(TokenKind kind, const char *message) {
  Token next = peek();

  if (next.kind == kind) {
    advance();
  } else {
    report_error_at(&next, message);
  }
}

static bool match(TokenKind kind) {
  if (peek().kind != kind) {
    return false;
  }

  advance();
  return true;
}

static void emit_byte(uint8_t byte) {
  chunk_write(current_chunk, byte, parser.last.line);
}

static void emit_const(Value value) {
  int idx = chunk_push_const(current_chunk, value);

  emit_byte(OP_LOAD);
  emit_byte((uint8_t)idx);
}

static uint8_t emit_identifier(Token *token) {
  ObjString *name = string_copy(token->start, token->len);
  return (uint8_t)chunk_push_const(current_chunk, OBJ_VAL(name));
}

static int emit_jump(OpCode instruction) {
  emit_byte(instruction);
  emit_byte(0xFF);
  emit_byte(0xFF);

  return current_chunk->len - 2;
}

static void patch_jump(int offset) {
  unsigned int distance = current_chunk->len - offset - 2;

  if (distance > UINT16_MAX) {
    report_error("Too much code to jump over.");
  }

  current_chunk->code[offset] = (distance >> 8) & 0xFF;
  current_chunk->code[offset + 1] = (distance) & 0xFF;
}

static void emit_jump_back(int start) {
  unsigned int distance = current_chunk->len - start + 3;

  if (distance > UINT16_MAX) {
    report_error("Too much code to jump over.");
  }

  emit_byte(OP_JUMP_BACK);
  emit_byte((distance >> 8) & 0xFF);
  emit_byte(distance & 0xFF);
}

// Forward declaration.
static void expression(void);
static void statement(void);
static void declaration(void);
static void declaration_variable(void);

static inline bool token_is_equal(Token *lhs, Token *rhs) {
  return (lhs->len == rhs->len) &&
         (memcmp(lhs->start, rhs->start, lhs->len) == 0);
}

static void scope_begin(void) {
  current->depth += 1;
}

static void scope_end(void) {
  current->depth -= 1;

  while (current->len > 0) {
    if (current->locals[current->len - 1].depth > current->depth) {
      break;
    }

    emit_byte(OP_POP);
    current->len--;
  }
}

static int scope_find_local(Compiler *compiler, Token *name) {
  for (int i = compiler->len - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];

    if (!token_is_equal(&local->name, name)) {
      continue;
    }

    if (local->depth == -1) {
      report_error("Can't read a variable in it's own initializer.");
    }

    return i;
  }

  return -1;
}

static void expression_variable(Token *name) {
  uint8_t set_op = 0;
  uint8_t get_op = 0;

  int idx = scope_find_local(current, name);

  if (idx == -1) {
    idx = emit_identifier(name);
    set_op = OP_SET_GLOBAL;
    get_op = OP_GET_GLOBAL;
  } else {
    set_op = OP_SET_LOCAL;
    get_op = OP_GET_LOCAL;
  }

  if (match(TOKEN_EQUAL)) {
    expression();
    emit_byte(set_op);
    emit_byte((uint8_t)idx);
  } else {
    emit_byte(get_op);
    emit_byte((uint8_t)idx);
  }
}

static void expression_primary(void) {
  Token next = advance();

  switch (next.kind) {
  case TOKEN_LEFT_PAREN:
    expression();
    expect(TOKEN_RIGHT_PAREN, "Expected closing parenthesis.");
    break;

  case TOKEN_NIL:
    emit_byte(OP_NIL);
    break;

  case TOKEN_TRUE:
    emit_byte(OP_TRUE);
    break;

  case TOKEN_FALSE:
    emit_byte(OP_FALSE);
    break;

  case TOKEN_NUMBER:
    emit_const(NUMBER_VAL(strtod(next.start, NULL)));
    break;

  case TOKEN_STRING:
    emit_const(OBJ_VAL(string_copy(next.start + 1, next.len - 2)));
    break;

  case TOKEN_IDENTIFIER:
    expression_variable(&next);
    break;

  default:
    report_error("Expected expression.");
    break;
  }
}

static void expression_unary(void) {
  switch (peek().kind) {
  case TOKEN_MINUS:
    advance();
    expression_primary();
    emit_byte(OP_NEGATE);
    break;

  case TOKEN_BANG:
    advance();
    expression_primary();
    emit_byte(OP_NOT);
    break;

  default:
    expression_primary();
  }
}

static void expression_factor(void) {
  expression_unary();

  while (match(TOKEN_STAR) || match(TOKEN_SLASH)) {
    OpCode op = (parser.last.kind == TOKEN_STAR) ? OP_MULTIPLY : OP_DIVIDE;
    expression_unary();
    emit_byte(op);
  }
}

static void expression_term(void) {
  expression_factor();

  while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
    OpCode op = (parser.last.kind == TOKEN_PLUS) ? OP_ADD : OP_SUBTRACT;
    expression_factor();
    emit_byte(op);
  }
}

static void expression_comparison(void) {
  expression_term();

  while (true) {
    switch (peek().kind) {
    case TOKEN_LESSER:
      advance();
      expression_term();
      emit_byte(OP_LESSER);
      continue;

    case TOKEN_LESSER_EQUAL:
      advance();
      expression_term();
      emit_byte(OP_GREATER);
      emit_byte(OP_NOT);
      continue;

    case TOKEN_GREATER:
      advance();
      expression_term();
      emit_byte(OP_GREATER);
      continue;

    case TOKEN_GREATER_EQUAL:
      advance();
      expression_term();
      emit_byte(OP_LESSER);
      emit_byte(OP_NOT);
      continue;

    case TOKEN_EQUAL_EQUAL:
      advance();
      expression_term();
      emit_byte(OP_EQUAL);
      continue;

    case TOKEN_BANG_EQUAL:
      advance();
      expression_term();
      emit_byte(OP_EQUAL);
      emit_byte(OP_NOT);
      continue;

    default:
      return;
    }
  }
}

static void expression_and(void) {
  expression_comparison();

  if (match(TOKEN_AND)) {
    int end_offset = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    expression_and();

    patch_jump(end_offset);
  }
}

static void expression_or(void) {
  expression_and();

  if (match(TOKEN_OR)) {
    int end_offset = emit_jump(OP_JUMP_IF_TRUE);

    emit_byte(OP_POP);
    expression_or();

    patch_jump(end_offset);
  }
}

static void expression(void) {
  expression_or();
}

static void statement_print(void) {
  advance();
  expression();
  expect(TOKEN_SEMICOLON, "Expected ; after value.");
  emit_byte(OP_PRINT);
}

static void statement_block(void) {
  advance();
  scope_begin();

  while (!match(TOKEN_RIGHT_BRACE) && !match(TOKEN_EOF)) {
    declaration();
  }

  if (parser.last.kind != TOKEN_RIGHT_BRACE) {
    report_error("Expected } after block.");
  }

  scope_end();
}

static void statement_expression(void) {
  expression();
  expect(TOKEN_SEMICOLON, "Expected ; after value.");
  emit_byte(OP_POP);
}

static void statement_if(void) {
  advance();
  expect(TOKEN_LEFT_PAREN, "Expected ( after 'if'.");
  expression();
  expect(TOKEN_RIGHT_PAREN, "Expected ) after condition.");

  int then_offset = emit_jump(OP_JUMP_IF_FALSE);

  emit_byte(OP_POP);
  statement();

  int else_offset = emit_jump(OP_JUMP);
  patch_jump(then_offset);

  emit_byte(OP_POP);
  if (match(TOKEN_ELSE)) {
    statement();
  }

  patch_jump(else_offset);
}

static void statement_while(void) {
  int start = current_chunk->len;

  advance();
  expect(TOKEN_LEFT_PAREN, "Expected ( after 'while'.");
  expression();
  expect(TOKEN_RIGHT_PAREN, "Expected ) after condition.");

  int exit_offset = emit_jump(OP_JUMP_IF_FALSE);

  emit_byte(OP_POP);
  statement();
  emit_jump_back(start);

  patch_jump(exit_offset);
  emit_byte(OP_POP);
}

static void statement_for(void) {
  advance();
  scope_begin();
  expect(TOKEN_LEFT_PAREN, "Expected ( after 'for'.");

  if (!match(TOKEN_SEMICOLON)) {
    if (match(TOKEN_LET)) {
      declaration_variable();
    } else {
      statement_expression();
    }
  }

  int loop_start = current_chunk->len;
  int exit_offset = -1;

  if (!match(TOKEN_SEMICOLON)) {
    expression();
    expect(TOKEN_SEMICOLON, "Expected ; after loop condition.");

    exit_offset = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int body_offset = emit_jump(OP_JUMP);
    int increment_start = current_chunk->len;

    expression();
    emit_byte(OP_POP);
    emit_jump_back(loop_start);
    loop_start = increment_start;
    patch_jump(body_offset);

    expect(TOKEN_RIGHT_PAREN, "Expected ) after loop clauses.");
  }

  statement();
  emit_jump_back(loop_start);

  if (exit_offset != -1) {
    patch_jump(exit_offset);
    emit_byte(OP_POP);
  }

  scope_end();
}

static void statement(void) {
  switch (peek().kind) {
  case TOKEN_LEFT_BRACE:
    statement_block();
    break;

  case TOKEN_PRINT:
    statement_print();
    break;

  case TOKEN_IF:
    statement_if();
    break;

  case TOKEN_WHILE:
    statement_while();
    break;

  case TOKEN_FOR:
    statement_for();
    break;

  default:
    statement_expression();
    break;
  }
}

static void add_local(Token name) {
  if (current->len > UINT8_MAX) {
    report_error("Too many local variables in the function.");
    return;
  }

  Local *local = &current->locals[current->len++];

  local->name = name;
  local->depth = -1;
}

static void declare_variable(void) {
  if (current->depth == 0) {
    return;
  }

  for (int i = current->len - 1; i >= 0; i--) {
    Local *local = &current->locals[i];

    if (local->depth != -1 && local->depth < current->depth) {
      break;
    }

    if (token_is_equal(&parser.last, &local->name)) {
      report_error("A variable with this name already exists in this scope.");
    }
  }

  add_local(parser.last);
}

static uint8_t variable(const char *message) {
  expect(TOKEN_IDENTIFIER, message);
  declare_variable();

  if (current->depth > 0) {
    return 0;
  }

  return emit_identifier(&parser.last);
}

static void define_variable(uint8_t global) {
  if (current->depth > 0) {
    current->locals[current->len - 1].depth = current->depth;
    return;
  }

  emit_byte(OP_DEFINE_GLOBAL);
  emit_byte(global);
}

static void declaration_variable(void) {
  uint8_t global = variable("Expected variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }

  expect(TOKEN_SEMICOLON, "Expected ; after variable declaration.");
  define_variable(global);
}

static void synchronize(void) {
  parser.panic = false;

  while (!match(TOKEN_EOF)) {
    switch (peek().kind) {
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
      advance();
      return;

    default:
      advance();
      continue;
    }
  }
}

static void declaration(void) {
  if (match(TOKEN_LET)) {
    declaration_variable();
  } else {
    statement();
  }

  if (parser.panic) {
    synchronize();
  }
}

bool compiler_compile(Chunk *chunk) {
  Compiler compiler = {.depth = 0, .len = 0};

  parser.had_error = false;
  parser.panic = false;
  current_chunk = chunk;
  current = &compiler;

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  emit_byte(OP_RETURN);
  return !parser.had_error;
}
