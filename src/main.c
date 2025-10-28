#include "chunk.h"
#include "vm.h"
#include <stdint.h>

int main(void) {
  vm_init();
  Chunk chunk;
  chunk_init(&chunk);

  chunk_write(&chunk, OP_LOAD, 1);
  chunk_write(&chunk, (uint8_t)chunk_push_const(&chunk, 1.2), 1);
  chunk_write(&chunk, OP_NEGATE, 1);
  chunk_write(&chunk, OP_LOAD, 1);
  chunk_write(&chunk, (uint8_t)chunk_push_const(&chunk, 3), 1);
  chunk_write(&chunk, OP_MULTIPLY, 2);
  chunk_write(&chunk, OP_RETURN, 2);

  chunk_print(&chunk, "MAIN");

  vm_interpret(&chunk);

  chunk_free(&chunk);
  vm_free();

  return 0;
}
