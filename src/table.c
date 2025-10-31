#include "table.h"
#include "mem.h"
#include "object.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

void table_init(Table *table) {
  table->len = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void table_free(Table *table) {
  MEM_FREE_ARRAY(Entry, table->entries, table->capacity);
  table_init(table);
}

static Entry *t_find_entry(Entry *entries, int capacity, ObjString *key) {
  uint32_t idx = key->hash % capacity;
  Entry *tombstone = NULL;

  while (true) {
    Entry *entry = &entries[idx];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        if (tombstone == NULL) {
          tombstone = entry;
        }
      }
    } else if (entry->key == key) {
      return entry;
    }

    idx = (idx + 1) % capacity;
  }
}

static void t_adjust_capacity(Table *table, int capacity) {
  Entry *entries = MEM_ALLOC(Entry, capacity);

  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  table->len = 0;

  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key == NULL) {
      continue;
    }

    Entry *dest = t_find_entry(entries, capacity, entry->key);

    dest->key = entry->key;
    dest->value = entry->value;
    table->len += 1;
  }

  MEM_FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool table_set(Table *table, struct ObjString *key, Value value) {
  if (table->len + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = MEM_GROW_CAPACITY(table->capacity);
    t_adjust_capacity(table, capacity);
  }

  Entry *entry = t_find_entry(table->entries, table->capacity, key);
  bool is_new = entry->key == NULL;

  if (is_new && IS_NIL(entry->value)) {
    table->len += 1;
  }

  entry->key = key;
  entry->value = value;

  return is_new;
}

bool table_get(Table *table, ObjString *key, Value *dest) {
  if (table->len == 0) {
    return false;
  }

  Entry *entry = t_find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }

  *dest = entry->value;
  return true;
}

bool table_remove(Table *table, ObjString *key) {
  if (table->len == 0) {
    return false;
  }

  Entry *entry = t_find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }

  entry->key = NULL;
  entry->value = BOOL_VAL(true);

  return true;
}
