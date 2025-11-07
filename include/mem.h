#ifndef MEM_H
#define MEM_H

#include "value.h"
#include <stdlib.h>

// #define STRESS_GC
// #define LOG_GC

#define MEM_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define MEM_GROW_ARRAY(type, ptr, old_len, new_len)                            \
  (type *)mem_realloc(ptr, sizeof(type) * (old_len), sizeof(type) * (new_len))
#define MEM_FREE_ARRAY(type, ptr, len)                                         \
  mem_realloc((void *)(ptr), sizeof(type) * (len), 0)

#define MEM_ALLOC(type, len) (type *)mem_realloc(NULL, 0, sizeof(type) * (len))
#define MEM_FREE(type, ptr) mem_realloc(ptr, sizeof(type), 0)

void *mem_realloc(void *ptr, size_t old_size, size_t new_size);

void collect_garbage(void);
void mark_object(Obj *object);

void object_free(Obj *object);

#endif
