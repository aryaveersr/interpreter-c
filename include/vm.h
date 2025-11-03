#ifndef VM_H
#define VM_H

#include "object.h"
#include "table.h"
#include "value.h"
#include <stdint.h>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * 256)

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

typedef struct {
  ObjFunction *function;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frames_len;

  Value *stack_top;
  Value stack[STACK_MAX];

  Obj *objects;
  Table strings;
  Table globals;
} VM;

extern VM vm;

void vm_init(void);
void vm_free(void);

void define_native(const char *name, NativeFn function);

InterpretResult vm_interpret(ObjFunction *function);

#endif
