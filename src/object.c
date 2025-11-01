#include "object.h"
#include "mem.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define ALLOC_OBJ(type, kind) (type *)object_alloc(sizeof(type), kind)

static Obj *object_alloc(size_t size, ObjKind kind) {
  Obj *object = (Obj *)mem_realloc(NULL, 0, size);

  object->kind = kind;
  object->next = vm.objects;
  vm.objects = object;

  return object;
}

static uint32_t hash_fnv1a(const char *chars, int len) {
  uint32_t hash = 2166136261U;

  for (int i = 0; i < len; i++) {
    hash ^= (uint8_t)chars[i];
    hash *= 16777619;
  }

  return hash;
}

static ObjString *strings_find(const char *chars, int len, uint32_t hash) {
  if (vm.strings.len == 0) {
    return NULL;
  }

  uint32_t idx = hash % vm.strings.capacity;
  while (true) {
    Entry *entry = &vm.strings.entries[idx];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        return NULL;
      }
    } else if (entry->key->len == len && entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, len) == 0) {
      return entry->key;
    }

    idx = (idx + 1) % vm.strings.capacity;
  }
}

ObjString *string_create(const char *chars, int len) {
  uint32_t hash = hash_fnv1a(chars, len);
  ObjString *string = strings_find(chars, len, hash);

  if (string != NULL) {
    MEM_FREE_ARRAY(char, chars, len);
    return string;
  }

  string = ALLOC_OBJ(ObjString, OBJ_STRING);

  string->len = len;
  string->chars = chars;
  string->hash = hash;

  table_set(&vm.strings, string, NIL_VAL);

  return string;
}

ObjString *string_copy(const char *chars, int len) {
  char *ptr = MEM_ALLOC(char, len + 1);
  memcpy(ptr, chars, len);
  ptr[len] = '\0';
  return string_create(ptr, len);
}
