#include "object.h"
#include "mem.h"
#include "value.h"
#include "vm.h"
#include <string.h>

#define ALLOC_OBJ(type, kind) (type *)object_alloc(sizeof(type), kind)

static Obj *object_alloc(size_t size, ObjKind kind) {
  Obj *object = (Obj *)mem_realloc(NULL, 0, size);

  object->kind = kind;
  object->next = vm.objects;
  vm.objects = object;

  return object;
}

ObjString *string_alloc(const char *chars, int len) {
  ObjString *string = ALLOC_OBJ(ObjString, OBJ_STRING);

  string->len = len;
  string->chars = chars;

  return string;
}

ObjString *string_copy(const char *chars, int len) {
  char *ptr = MEM_ALLOC(char, len + 1);
  memcpy(ptr, chars, len);
  ptr[len] = '\0';
  return string_alloc(ptr, len);
}
