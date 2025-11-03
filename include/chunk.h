#ifndef CHUNK_H
#define CHUNK_H

#include "value.h"
#include "value_list.h"
#include <stdint.h>

typedef enum {
  OP_RETURN,
  OP_LOAD,

  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,

  OP_NOT,
  OP_LESSER,
  OP_GREATER,
  OP_EQUAL,

  OP_NIL,
  OP_TRUE,
  OP_FALSE,

  OP_PRINT,
  OP_POP,
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_LOCAL,
  OP_SET_LOCAL,

  OP_JUMP,
  OP_JUMP_BACK,
  OP_JUMP_IF_TRUE,
  OP_JUMP_IF_FALSE,
} OpCode;

typedef struct {
  int len;
  int capacity;
  uint8_t *code;

  ValueList consts;

  int lines_len;
  int lines_capacity;
  uint16_t *lines;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_free(Chunk *chunk);

void chunk_write(Chunk *chunk, uint8_t byte, uint16_t line);
int chunk_push_const(Chunk *chunk, Value value);

int chunk_get_line(Chunk *chunk, int offset);
int chunk_print_instr(Chunk *chunk, int offset);
void chunk_print(Chunk *chunk, const char *name);

#endif
