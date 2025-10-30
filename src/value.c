#include "value.h"
#include "object.h"
#include <string.h>

bool value_is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool value_is_equal(Value a, Value b) {
  if (a.kind != b.kind) {
    return false;
  }

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wswitch-enum"
  switch (a.kind) {
  case VAL_NIL:
    return true;

  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);

  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);

  case VAL_OBJ: {
    ObjString *a_str = AS_STRING(a);
    ObjString *b_str = AS_STRING(b);

    return (a_str->len == b_str->len) &&
           (memcmp(a_str->chars, b_str->chars, a_str->len) == 0);
  }
  }
#pragma clang diagnostic pop
}
