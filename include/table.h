#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>

#include "value.h"

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  int len;
  int capacity;
  Entry* entries;
} Table;

void table_init(Table* table);
void table_free(Table* table);

bool table_set(Table* table, ObjString* key, Value value);
bool table_get(Table* table, ObjString* key, Value* dest);
bool table_remove(Table* table, ObjString* key);
void table_add_all(Table* src, Table* dest);

#endif
