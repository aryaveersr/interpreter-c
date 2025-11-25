#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>

#include "value.h"
#include "value_list.h"

typedef struct {
  int len;
  int capacity;
  uint8_t* code;

  ValueList consts;

  int lines_len;
  int lines_capacity;
  uint16_t* lines;
} Chunk;

void chunk_init(Chunk* chunk);
void chunk_free(Chunk* chunk);

void chunk_write(Chunk* chunk, uint8_t byte, uint16_t line);
int chunk_push_const(Chunk* chunk, Value value);

int chunk_get_line(Chunk* chunk, int offset);
int chunk_print_instr(Chunk* chunk, int offset);
void chunk_print(Chunk* chunk, const char* name);

#endif
