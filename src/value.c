#include "value.h"

bool value_is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool value_is_equal(Value a, Value b) {
  if (a.kind != b.kind) {
    return false;
  }

  switch (a.kind) {
    case VAL_NIL:
      return true;

    case VAL_BOOL:
      return AS_BOOL(a) == AS_BOOL(b);

    case VAL_NUMBER:
      return AS_NUMBER(a) == AS_NUMBER(b);

    case VAL_OBJ:
      return AS_OBJ(a) == AS_OBJ(b);
  }
}
