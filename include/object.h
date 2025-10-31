#ifndef OBJECT_H
#define OBJECT_H

#include "value.h"
#include <stdbool.h>
#include <stdint.h>

#define OBJ_KIND(value) (AS_OBJ(value)->kind)

#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

#define IS_STRING(value) obj_is_kind(value, OBJ_STRING)

typedef enum {
  OBJ_STRING,
} ObjKind;

struct Obj {
  ObjKind kind;
  struct Obj *next;
};

struct ObjString {
  Obj obj;
  int len;
  const char *chars;
  uint32_t hash;
};

ObjString *string_create(const char *chars, int len);
ObjString *string_copy(const char *chars, int len);

static inline bool obj_is_kind(Value value, ObjKind kind) {
  return IS_OBJ(value) && (AS_OBJ(value)->kind == kind);
}

#endif
