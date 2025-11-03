#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "value.h"
#include <stdbool.h>
#include <stdint.h>

#define OBJ_KIND(value) (AS_OBJ(value)->kind)

#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNativeFn *)AS_OBJ(value))->function)

#define IS_STRING(value) obj_is_kind(value, OBJ_STRING)
#define IS_FUNCTION(value) obj_is_kind(value, OBJ_FUNCTION)
#define IS_NATIVE(value) obj_is_kind(value, OBJ_NATIVE_FN)

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE_FN,
} ObjKind;

struct Obj {
  ObjKind kind;
  struct Obj *next;
};

typedef struct {
  Obj obj;
  int len;
  const char *chars;
  uint32_t hash;
} ObjString;

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int arg_len, Value *args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNativeFn;

ObjString *string_new(const char *chars, int len);
ObjString *string_copy(const char *chars, int len);
ObjFunction *function_new(void);
ObjNativeFn *native_new(NativeFn function);

static inline bool obj_is_kind(Value value, ObjKind kind) {
  return IS_OBJ(value) && (AS_OBJ(value)->kind == kind);
}

#endif
