#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "table.h"
#include "value.h"
#include <stdint.h>

#define TRACE_VM
#define STACK_MAX 256

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

typedef struct {
  Chunk *chunk;
  uint8_t *ip;

  Value *stack_top;
  Value stack[STACK_MAX];

  Obj *objects;
  Table strings;
  Table globals;
} VM;

extern VM vm;

void vm_init(void);
void vm_free(void);

InterpretResult vm_interpret(Chunk *chunk);

#endif
