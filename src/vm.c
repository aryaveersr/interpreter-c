#include "vm.h"
#include "chunk.h"
#include "value.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

VM vm;

void vm_init(void) {
  vm.stack_top = vm.stack;
}

void vm_free(void) {}

static void stack_push(Value value) {
  *vm.stack_top = value;
  vm.stack_top += 1;
}

static Value stack_pop(void) {
  vm.stack_top -= 1;
  return *vm.stack_top;
}

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->consts.values[READ_BYTE()])

#define BINARY_OP(label, op)                                                   \
  case label: {                                                                \
    double b = stack_pop();                                                    \
    double a = stack_pop();                                                    \
                                                                               \
    stack_push(a op b);                                                        \
    break;                                                                     \
  }

InterpretResult vm_interpret(Chunk *chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;

  while (true) {
#ifdef TRACE_VM

    printf("  [STACK] ");

    for (Value *slot = vm.stack; slot < vm.stack_top; slot += 1) {
      printf("[ ");
      value_print(*slot);
      printf(" ]");
    }

    printf("\n");
    chunk_print_instr(vm.chunk, (int)(vm.ip - vm.chunk->code));

#endif

    uint8_t instr = READ_BYTE();

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wswitch-enum"

    switch ((OpCode)instr) {
      BINARY_OP(OP_ADD, +);
      BINARY_OP(OP_SUBTRACT, -);
      BINARY_OP(OP_MULTIPLY, *);
      BINARY_OP(OP_DIVIDE, /);

    case OP_RETURN:
      value_print(stack_pop());
      printf("\n");
      return INTERPRET_OK;

    case OP_LOAD:
      stack_push(READ_CONSTANT());
      break;

    case OP_NEGATE:
      stack_push(-stack_pop());
      break;

    default:
      printf("Unknown opcode: '%d'.\n", instr);
      return INTERPRET_RUNTIME_ERROR;
    }

#pragma clang diagnostic pop
  }
}

#undef READ_BYTE
#undef READ_CONSTANT
