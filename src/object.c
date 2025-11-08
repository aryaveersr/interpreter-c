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

  uint32_t idx = hash & (vm.strings.capacity - 1);
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

    idx = (idx + 1) & (vm.strings.capacity - 1);
  }
}

void object_free(Obj *object) {
#ifdef LOG_GC
  printf("-- %p free type %d\n", (void *)object, object->kind);
#endif

  switch (object->kind) {
  case OBJ_STRING: {
    ObjString *string = (ObjString *)object;
    MEM_FREE_ARRAY(char, string->chars, string->len + 1);
    MEM_FREE(ObjString, object);
    break;
  }

  case OBJ_FUNCTION: {
    ObjFunction *function = (ObjFunction *)object;
    chunk_free(&function->chunk);
    MEM_FREE(ObjFunction, object);
    break;
  }

  case OBJ_NATIVE_FN:
    MEM_FREE(ObjNativeFn, object);
    break;

  case OBJ_CLOSURE:
    MEM_FREE_ARRAY(ObjUpvalue *, ((ObjClosure *)object)->upvalues,
                   ((ObjClosure *)object)->upvalue_len);
    MEM_FREE(ObjClosure, object);
    break;

  case OBJ_UPVALUE:
    MEM_FREE(ObjUpvalue, object);
    break;

  case OBJ_CLASS:
    table_free(&((ObjClass *)object)->methods);
    MEM_FREE(ObjClass, object);
    break;

  case OBJ_INSTANCE: {
    ObjInstance *instance = (ObjInstance *)object;
    table_free(&instance->fields);
    MEM_FREE(ObjInstance, object);
    break;
  }

  case OBJ_BOUND_METHOD:
    MEM_FREE(ObjBoundMethod, object);
    break;
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

ObjClass *class_new(ObjString *name) {
  ObjClass *class = ALLOC_OBJ(ObjClass, OBJ_CLASS);

  class->name = name;
  table_init(&class->methods);

  return class;
}

ObjInstance *instance_new(ObjClass *class) {
  ObjInstance *instance = ALLOC_OBJ(ObjInstance, OBJ_INSTANCE);

  instance->class = class;
  table_init(&instance->fields);

  return instance;
}

ObjBoundMethod *boundmethod_new(Value receiver, ObjClosure *method) {
  ObjBoundMethod *bound = ALLOC_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);

  bound->receiver = receiver;
  bound->method = method;

  return bound;
}
