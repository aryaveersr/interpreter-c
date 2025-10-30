#include "mem.h"
#include "object.h"
#include "value.h"
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

void object_free(Obj *object) {
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wswitch-enum"
  switch (object->kind) {
  case OBJ_STRING: {
    ObjString *string = (ObjString *)object;
    MEM_FREE_ARRAY(char, string->chars, string->len + 1);
    MEM_FREE(ObjString, object);
    break;
  }
  }
#pragma clang diagnostic pop
}
