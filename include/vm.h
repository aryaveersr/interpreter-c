#ifndef VM_H
#define VM_H

#include "object.h"
#include "table.h"
#include "value.h"
#include <stdint.h>
#include <stdlib.h>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * 256)

// #define TRACE_VM

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

typedef struct {
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frames_len;

  Value *stack_top;
  Value stack[STACK_MAX];

  Obj *objects;
  ObjUpvalue *open_upvalues;
  Table strings;
  ObjString *init_string;
  Table globals;

  int gray_len;
  int gray_capacity;
  Obj **gray_stack;

  size_t bytes_allocated;
  size_t gc_target;
} VM;

extern VM vm;

void vm_init(void);
void vm_free(void);

void push(Value value);
Value pop(void);

void define_native(const char *name, NativeFn function);

InterpretResult vm_interpret(ObjFunction *function);

#endif
