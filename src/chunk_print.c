#include "chunk.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>

static int instruction_const(const char *name, Chunk *chunk, int offset) {
  uint8_t idx = chunk->code[offset + 1];

  printf("%-16s %4d '", name, idx);
  value_print(chunk->consts.values[idx]);
  printf("'\n");

  return offset + 2;
}

#define SIMPLE_INSTR(name)                                                     \
  case name:                                                                   \
    printf(#name "\n");                                                        \
    return offset + 1

int chunk_print_instr(Chunk *chunk, int offset) {
  printf("%04d %4d ", offset, chunk_get_line(chunk, offset));
  uint8_t instr = chunk->code[offset];

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wswitch-enum"
  switch ((OpCode)instr) {
    SIMPLE_INSTR(OP_RETURN);

    SIMPLE_INSTR(OP_NEGATE);
    SIMPLE_INSTR(OP_ADD);
    SIMPLE_INSTR(OP_SUBTRACT);
    SIMPLE_INSTR(OP_MULTIPLY);
    SIMPLE_INSTR(OP_DIVIDE);

    SIMPLE_INSTR(OP_NOT);
    SIMPLE_INSTR(OP_LESSER);
    SIMPLE_INSTR(OP_GREATER);
    SIMPLE_INSTR(OP_EQUAL);

    SIMPLE_INSTR(OP_NIL);
    SIMPLE_INSTR(OP_TRUE);
    SIMPLE_INSTR(OP_FALSE);

  case OP_LOAD:
    return instruction_const("OP_LOAD", chunk, offset);

  default:
    printf("Unknown opcode: '%d'.\n", instr);
    return offset + 1;
  }
#pragma clang diagnostic pop
#undef SIMPLE_INSTR
}

void chunk_print(Chunk *chunk, const char *name) {
  printf("== %s (length: %05d) ==\n", name, chunk->len);

  for (int offset = 0; offset < chunk->len;) {
    offset = chunk_print_instr(chunk, offset);
  }
}
