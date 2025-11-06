#include "object.h"
#include "chunk.h"
#include "mem.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ALLOC_OBJ(type, kind) (type *)object_alloc(sizeof(type), kind)

static Obj *object_alloc(size_t size, ObjKind kind) {
  Obj *object = (Obj *)mem_realloc(NULL, 0, size);

  object->kind = kind;
  object->is_marked = false;
  object->next = vm.objects;
  vm.objects = object;

#ifdef LOG_GC
  printf("-- %p allocated %zu for %d\n", (void *)object, size, kind);
#endif

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

ObjString *string_new(const char *chars, int len) {
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

  push(OBJ_VAL(string));
  table_set(&vm.strings, string, NIL_VAL);
  pop();

  return string;
}

ObjString *string_copy(const char *chars, int len) {
  char *ptr = MEM_ALLOC(char, len + 1);
  memcpy(ptr, chars, len);
  ptr[len] = '\0';
  return string_new(ptr, len);
}

ObjFunction *function_new(void) {
  ObjFunction *function = ALLOC_OBJ(ObjFunction, OBJ_FUNCTION);

  function->arity = 0;
  function->name = NULL;
  function->upvalue_len = 0;

  chunk_init(&function->chunk);

  return function;
}

ObjNativeFn *native_new(NativeFn function) {
  ObjNativeFn *native = ALLOC_OBJ(ObjNativeFn, OBJ_NATIVE_FN);
  native->function = function;
  return native;
}

ObjClosure *closure_new(ObjFunction *function) {
  ObjUpvalue **upvalues = MEM_ALLOC(ObjUpvalue *, function->upvalue_len);

  for (int i = 0; i < function->upvalue_len; i++) {
    upvalues[i] = NULL;
  }

  ObjClosure *closure = ALLOC_OBJ(ObjClosure, OBJ_CLOSURE);

  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_len = function->upvalue_len;

  return closure;
}

ObjUpvalue *upvalue_new(Value *slot) {
  ObjUpvalue *upvalue = ALLOC_OBJ(ObjUpvalue, OBJ_UPVALUE);

  upvalue->ptr = slot;
  upvalue->next = NULL;
  upvalue->closed = NIL_VAL;

  return upvalue;
}
