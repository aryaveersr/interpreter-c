#include "chunk.h"
#include "mem.h"
#include "value_list.h"
#include "vm.h"
#include <stdint.h>
#include <stdlib.h>

void chunk_init(Chunk *chunk) {
  chunk->len = 0;
  chunk->capacity = 0;
  chunk->code = NULL;

  valuelist_init(&chunk->consts);

  chunk->lines_len = 0;
  chunk->lines_capacity = 0;
  chunk->lines = NULL;
}

void chunk_free(Chunk *chunk) {
  MEM_FREE_ARRAY(uint8_t, chunk->code, chunk->len);
  valuelist_free(&chunk->consts);

  chunk_init(chunk);
}

void chunk_write(Chunk *chunk, uint8_t byte, uint16_t line) {
  if (chunk->capacity < chunk->len + 1) {
    int old_capacity = chunk->capacity;

    chunk->capacity = MEM_GROW_CAPACITY(old_capacity);
    chunk->code =
        MEM_GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
  }

  chunk->code[chunk->len] = byte;
  chunk->len += 1;

  if ((chunk->lines == NULL) || (chunk->lines[chunk->lines_len - 2] != line) ||
      (chunk->lines[chunk->lines_len - 1] == UINT16_MAX)) {
    if ((chunk->lines == NULL) ||
        (chunk->lines_capacity < chunk->lines_len + 2)) {
      int old_capacity = chunk->lines_capacity;

      chunk->lines_capacity = MEM_GROW_CAPACITY(old_capacity);
      chunk->lines = MEM_GROW_ARRAY(uint16_t, chunk->lines, old_capacity,
                                    chunk->lines_capacity);
    }

    chunk->lines[chunk->lines_len] = line;
    chunk->lines[chunk->lines_len + 1] = 0;
    chunk->lines_len += 2;
  } else {
    chunk->lines[chunk->lines_len - 1] += 1;
  }
}

int chunk_push_const(Chunk *chunk, Value value) {
  push(value);
  valuelist_write(&chunk->consts, value);
  pop();
  return chunk->consts.len - 1;
}

int chunk_get_line(Chunk *chunk, int offset) {
  int acc_offset = 0;

  for (int i = 0; i < chunk->lines_len && acc_offset <= offset; i += 2) {
    acc_offset += chunk->lines[i + 1] + 1;

    if (acc_offset > offset) {
      return chunk->lines[i];
    }
  }

  return -1;
}
