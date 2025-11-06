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
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_UPVALUE(value) ((ObjUpvalue *)AS_OBJ(value))

#define IS_STRING(value) obj_is_kind(value, OBJ_STRING)
#define IS_FUNCTION(value) obj_is_kind(value, OBJ_FUNCTION)
#define IS_NATIVE(value) obj_is_kind(value, OBJ_NATIVE_FN)
#define IS_CLOSURE(value) obj_is_kind(value, OBJ_CLOSURE)

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE_FN,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
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
  uint8_t idx;
  bool is_local;
} Upvalue;

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString *name;
  int upvalue_len;
} ObjFunction;

typedef Value (*NativeFn)(int arg_len, Value *args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNativeFn;

typedef struct ObjUpvalue {
  Obj obj;
  Value *ptr;
  struct ObjUpvalue *next;
  Value closed;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  int upvalue_len;
} ObjClosure;

ObjString *string_copy(const char *chars, int len);
ObjString *string_new(const char *chars, int len);
ObjFunction *function_new(void);
ObjNativeFn *native_new(NativeFn function);
ObjClosure *closure_new(ObjFunction *function);
ObjUpvalue *upvalue_new(Value *slot);

static inline bool obj_is_kind(Value value, ObjKind kind) {
  return IS_OBJ(value) && (AS_OBJ(value)->kind == kind);
}

#endif
