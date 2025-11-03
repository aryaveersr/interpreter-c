#include "object.h"
#include "value.h"
#include <stdio.h>

static void object_print(Value value) {
  switch (OBJ_KIND(value)) {
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;

  case OBJ_FUNCTION: {
    ObjFunction *function = AS_FUNCTION(value);

    if (function->name == NULL) {
      printf("<script>");
    } else {
      printf("<function '%s'(%d)>", function->name->chars, function->arity);
    }

    break;
  }

  case OBJ_NATIVE_FN:
    printf("<native function>");
    break;
  }
}

void value_print(Value value) {
  switch (value.kind) {
  case VAL_NIL:
    printf("nil");
    break;

  case VAL_BOOL:
    printf(AS_BOOL(value) ? "true" : "false");
    break;

  case VAL_NUMBER:
    printf("%g", AS_NUMBER(value));
    break;

  case VAL_OBJ:
    object_print(value);
    break;
  }
}
