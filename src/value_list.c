#include "value_list.h"
#include "mem.h"
#include "value.h"
#include <stdlib.h>

void valuelist_init(ValueList *list) {
  list->len = 0;
  list->capacity = 0;
  list->values = NULL;
}

void valuelist_free(ValueList *list) {
  MEM_FREE_ARRAY(Value, list->values, list->len);
  valuelist_init(list);
}

void valuelist_write(ValueList *list, Value value) {
  if (list->capacity < list->len + 1) {
    int old_capacity = list->capacity;

    list->capacity = MEM_GROW_CAPACITY(old_capacity);
    list->values =
        MEM_GROW_ARRAY(Value, list->values, old_capacity, list->capacity);
  }

  list->values[list->len] = value;
  list->len += 1;
}
