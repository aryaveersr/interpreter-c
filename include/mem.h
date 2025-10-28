#ifndef MEM_H
#define MEM_H

#include <stdlib.h>

#define MEM_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define MEM_GROW_ARRAY(type, ptr, old_len, new_len)                            \
  (type *)mem_realloc(ptr, sizeof(type) * (old_len), sizeof(type) * (new_len))

#define MEM_FREE_ARRAY(type, ptr, len) mem_realloc(ptr, sizeof(type) * (len), 0)

void *mem_realloc(void *ptr, size_t old_size, size_t new_size);

#endif
