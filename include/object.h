#ifndef OBJECT_H
#define OBJECT_H

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_KIND(value) (AS_OBJ(value)->kind)

#define AS_STRING(value) ((ObjString*) AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*) AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction*) AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNativeFn*) AS_OBJ(value))->function)
#define AS_CLOSURE(value) ((ObjClosure*) AS_OBJ(value))
#define AS_UPVALUE(value) ((ObjUpvalue*) AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*) AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*) AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*) AS_OBJ(value))

#define IS_STRING(value) obj_is_kind(value, OBJ_STRING)
#define IS_FUNCTION(value) obj_is_kind(value, OBJ_FUNCTION)
#define IS_NATIVE(value) obj_is_kind(value, OBJ_NATIVE_FN)
#define IS_CLOSURE(value) obj_is_kind(value, OBJ_CLOSURE)
#define IS_CLASS(value) obj_is_kind(value, OBJ_CLASS)
#define IS_INSTANCE(value) obj_is_kind(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) obj_is_kind(value, OBJ_BOUND_METHOD)

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE_FN,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
  OBJ_CLASS,
  OBJ_INSTANCE,
  OBJ_BOUND_METHOD,
} ObjKind;

struct Obj {
  ObjKind kind;
  bool is_marked;
  struct Obj* next;
};

struct ObjString {
  Obj obj;
  int len;
  const char* chars;
  uint32_t hash;
};

typedef struct {
  uint8_t idx;
  bool is_local;
} Upvalue;

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString* name;
  int upvalue_len;
} ObjFunction;

typedef Value (*NativeFn)(int arg_len, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNativeFn;

typedef struct ObjUpvalue {
  Obj obj;
  Value* ptr;
  struct ObjUpvalue* next;
  Value closed;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalue_len;
} ObjClosure;

typedef struct {
  Obj obj;
  ObjString* name;
  Table methods;
} ObjClass;

typedef struct {
  Obj obj;
  ObjClass* class;
  Table fields;
} ObjInstance;

typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

void object_free(Obj* object);

ObjString* string_copy(const char* chars, int len);
ObjString* string_new(const char* chars, int len);
ObjFunction* function_new(void);
ObjNativeFn* native_new(NativeFn function);
ObjClosure* closure_new(ObjFunction* function);
ObjUpvalue* upvalue_new(Value* slot);
ObjClass* class_new(ObjString* name);
ObjInstance* instance_new(ObjClass* class);
ObjBoundMethod* boundmethod_new(Value receiver, ObjClosure* method);

__attribute__((unused)) //
static inline bool
obj_is_kind(Value value, ObjKind kind) {
  return IS_OBJ(value) && (AS_OBJ(value)->kind == kind);
}

#endif
