#include "mem.h"
#include <stdlib.h>

void *mem_realloc(void *ptr, size_t old_size, size_t new_size) {
  (void)old_size;

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);

  if (result == NULL) {
    exit(EXIT_FAILURE);
  }

  return result;
}
