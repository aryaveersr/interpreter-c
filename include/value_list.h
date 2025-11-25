#ifndef VALUE_LIST_H
#define VALUE_LIST_H

#include "value.h"

typedef struct {
  int len;
  int capacity;
  Value* values;
} ValueList;

void valuelist_init(ValueList* list);
void valuelist_free(ValueList* list);

void valuelist_write(ValueList* list, Value value);

#endif
