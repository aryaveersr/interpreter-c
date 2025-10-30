#include "value.h"
#include <stdio.h>

void value_print(Value value) {
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wswitch-enum"
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
  }
#pragma clang diagnostic pop
}
