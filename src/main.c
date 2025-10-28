#include "chunk.h"
#include <stdint.h>

int main(void) {
  Chunk chunk;

  chunk_init(&chunk);

  int idx = chunk_push_const(&chunk, 1.2);

  chunk_write(&chunk, OP_RETURN, 1);
  chunk_write(&chunk, OP_LOAD, 1);
  chunk_write(&chunk, (uint8_t)idx, 1);
  chunk_write(&chunk, OP_RETURN, 2);

  chunk_print(&chunk, "MAIN");

  chunk_free(&chunk);

  return 0;
}
