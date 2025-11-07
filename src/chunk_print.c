#include "chunk.h"
#include "object.h"
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

static int instruction_byte(const char *name, Chunk *chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

static int instruction_jump(const char *name, int sign, Chunk *chunk,
                            int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8) |
                  (uint16_t)chunk->code[offset + 2];

  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);

  return offset + 3;
}

#define SIMPLE_INSTR(name)                                                     \
  case name:                                                                   \
    printf(#name "\n");                                                        \
    return offset + 1

#define CONST_INSTR(name)                                                      \
  case name:                                                                   \
    return instruction_const(#name, chunk, offset)

#define BYTE_INSTR(name)                                                       \
  case name:                                                                   \
    return instruction_byte(#name, chunk, offset)

#define JUMP_INSTR(name, sign)                                                 \
  case name:                                                                   \
    return instruction_jump(#name, sign, chunk, offset)

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

    SIMPLE_INSTR(OP_PRINT);
    SIMPLE_INSTR(OP_POP);

    SIMPLE_INSTR(OP_CLOSE_UPVALUE);

    CONST_INSTR(OP_LOAD);
    CONST_INSTR(OP_DEFINE_GLOBAL);
    CONST_INSTR(OP_GET_GLOBAL);
    CONST_INSTR(OP_SET_GLOBAL);

    CONST_INSTR(OP_CLASS);
    CONST_INSTR(OP_GET_PROPERTY);
    CONST_INSTR(OP_SET_PROPERTY);

    BYTE_INSTR(OP_GET_LOCAL);
    BYTE_INSTR(OP_SET_LOCAL);
    BYTE_INSTR(OP_GET_UPVALUE);
    BYTE_INSTR(OP_SET_UPVALUE);

    BYTE_INSTR(OP_CALL);

    JUMP_INSTR(OP_JUMP, 1);
    JUMP_INSTR(OP_JUMP_BACK, -1);
    JUMP_INSTR(OP_JUMP_IF_TRUE, 1);
    JUMP_INSTR(OP_JUMP_IF_FALSE, 1);

  case OP_CLOSURE: {
    offset++;
    uint8_t idx = chunk->code[offset++];

    printf("%-16s %4d ", "OP_CLOSURE", idx);
    value_print(chunk->consts.values[idx]);
    printf("\n");

    ObjFunction *function = AS_FUNCTION(chunk->consts.values[idx]);

    for (int i = 0; i < function->upvalue_len; i++) {
      int is_local = chunk->code[offset++];
      int idx = chunk->code[offset++];

      printf("%04d      |                     %s %d\n", offset - 2,
             is_local ? "Local" : "Upvalue", idx);
    }

    return offset;
  }

  default:
    printf("Unknown opcode: '%d'.\n", instr);
    return offset + 1;
  }
#pragma clang diagnostic pop
#undef SIMPLE_INSTR
}

void chunk_print(Chunk *chunk, const char *name) {
  printf("== %s (length: %03d) ==\n", name, chunk->len);

  for (int offset = 0; offset < chunk->len;) {
    offset = chunk_print_instr(chunk, offset);
  }
}
