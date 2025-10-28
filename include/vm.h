#ifndef VM_H
#define VM_H

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
} VM;

void vm_init(void);
void vm_free(void);

InterpretResult vm_interpret(Chunk *chunk);

#endif
