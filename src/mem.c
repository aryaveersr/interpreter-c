#include "mem.h"
#include "chunk.h"
#include "compiler.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "value_list.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GC_HEAP_GROW_FACTOR 2

void *mem_realloc(void *ptr, size_t old_size, size_t new_size) {
  vm.bytes_allocated += new_size - old_size;

  if (new_size > old_size) {
#ifdef STRESS_GC
    collect_garbage();
#endif
  }

  if (vm.bytes_allocated > vm.gc_target) {
    collect_garbage();
  }

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);

  if (result == NULL) {
    exit(EXIT_FAILURE);
  }

  return result;
}

void object_free(Obj *object) {
#ifdef LOG_GC
  printf("-- %p free type %d\n", (void *)object, object->kind);
#endif

  switch (object->kind) {
  case OBJ_STRING: {
    ObjString *string = (ObjString *)object;
    MEM_FREE_ARRAY(char, string->chars, string->len + 1);
    MEM_FREE(ObjString, object);
    break;
  }

  case OBJ_FUNCTION: {
    ObjFunction *function = (ObjFunction *)object;
    chunk_free(&function->chunk);
    MEM_FREE(ObjFunction, object);
    break;
  }

  case OBJ_NATIVE_FN:
    MEM_FREE(ObjNativeFn, object);
    break;

  case OBJ_CLOSURE:
    MEM_FREE_ARRAY(ObjUpvalue *, ((ObjClosure *)object)->upvalues,
                   ((ObjClosure *)object)->upvalue_len);
    MEM_FREE(ObjClosure, object);
    break;

  case OBJ_UPVALUE:
    MEM_FREE(ObjUpvalue, object);
    break;
  }
}

static void mark_value(Value value);

static void mark_table(Table *table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];

    mark_object((Obj *)entry->key);
    mark_value(entry->value);
  }
}

void mark_object(Obj *object) {
  if (object == NULL || object->is_marked) {
    return;
  }

#ifdef LOG_GC
  printf("-- %p mark ", (void *)object);
  value_print(OBJ_VAL(object));
  printf("\n");
#endif

  object->is_marked = true;

  if (vm.gray_capacity < vm.gray_len + 1) {
    vm.gray_capacity = MEM_GROW_CAPACITY(vm.gray_capacity);
    Obj **new_stack =
        (Obj **)realloc(vm.gray_stack, sizeof(Obj *) * vm.gray_capacity);

    if (new_stack == NULL) {
      free(vm.gray_stack);
      exit(EXIT_FAILURE);
    }

    vm.gray_stack = new_stack;
  }

  vm.gray_stack[vm.gray_len++] = object;
}

static void mark_value(Value value) {
  if (IS_OBJ(value)) {
    mark_object(AS_OBJ(value));
  }
}

static void mark_roots(void) {
  for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
    mark_value(*slot);
  }

  for (int i = 0; i < vm.frames_len; i++) {
    mark_object((Obj *)vm.frames[i].closure);
  }

  for (ObjUpvalue *upvalue = vm.open_upvalues; upvalue != NULL;
       upvalue = upvalue->next) {
    mark_object((Obj *)upvalue);
  }

  mark_table(&vm.globals);
  mark_compiler_roots();
}

static void mark_valuelist(ValueList *list) {
  for (int i = 0; i < list->len; i++) {
    mark_value(list->values[i]);
  }
}

static void blacken_object(Obj *object) {
#ifdef LOG_GC
  printf("-- %p blacken ", (void *)object);
  value_print(OBJ_VAL(object));
  printf("\n");
#endif

  switch (object->kind) {
  case OBJ_NATIVE_FN:
  case OBJ_STRING:
    break;

  case OBJ_UPVALUE:
    mark_value(((ObjUpvalue *)object)->closed);
    break;

  case OBJ_FUNCTION: {
    ObjFunction *function = (ObjFunction *)object;
    mark_object((Obj *)function->name);
    mark_valuelist(&function->chunk.consts);
    break;
  }

  case OBJ_CLOSURE: {
    ObjClosure *closure = (ObjClosure *)object;
    mark_object((Obj *)closure->function);

    for (int i = 0; i < closure->upvalue_len; i++) {
      mark_object((Obj *)closure->upvalues[i]);
    }

    break;
  }
  }
}

static void trace_references(void) {
  while (vm.gray_len > 0) {
    Obj *object = vm.gray_stack[--vm.gray_len];
    blacken_object(object);
  }
}

static void sweep(void) {
  Obj *prev = NULL;
  Obj *object = vm.objects;

  while (object != NULL) {
    if (object->is_marked) {
      object->is_marked = false;
      prev = object;
      object = object->next;
      continue;
    }

    Obj *unreached = object;
    object = object->next;

    if (prev != NULL) {
      prev->next = object;
    } else {
      vm.objects = object;
    }

    object_free(unreached);
  }
}

static void table_remove_unreachable(Table *table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];

    if (entry->key != NULL && !entry->key->obj.is_marked) {
      table_remove(table, entry->key);
    }
  }
}

void collect_garbage(void) {
#ifdef LOG_GC
  printf("-- GC Begin\n");
  size_t starting_size = vm.bytes_allocated;
#endif

  mark_roots();
  trace_references();
  table_remove_unreachable(&vm.strings);
  sweep();

  vm.gc_target = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef LOG_GC
  printf("-- GC End\n");
  printf("-- collected %zu bytes (from %zu to %zu) next at %zu\n",
         starting_size - vm.bytes_allocated, starting_size, vm.bytes_allocated,
         vm.gc_target);
#endif
}
