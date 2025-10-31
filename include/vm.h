#ifndef VM_H
#define VM_H

#include "table.h"
#define TRACE_VM
#define STACK_MAX 256

#include "chunk.h"
#include "value.h"
#include <stdint.h>

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
} VM;

extern VM vm;

void vm_init(void);
void vm_free(void);

InterpretResult vm_interpret(Chunk *chunk);

#endif
