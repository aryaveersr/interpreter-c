#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>

#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define IS_NIL(value) ((value).kind == VAL_NIL)
#define IS_BOOL(value) ((value).kind == VAL_BOOL)
#define IS_NUMBER(value) ((value).kind == VAL_NUMBER)

typedef enum {
  VAL_NIL,
  VAL_BOOL,
  VAL_NUMBER,
} ValueKind;

typedef struct {
  ValueKind kind;
  union {
    bool boolean;
    double number;
  } as;
} Value;

bool value_is_falsey(Value value);
bool value_is_equal(Value a, Value b);

void value_print(Value value);

#endif
