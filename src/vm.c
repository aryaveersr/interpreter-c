#include "vm.h"
#include "chunk.h"
#include "mem.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

VM vm;

void vm_init(void) {
  vm.stack_top = vm.stack;
  vm.objects = NULL;

  table_init(&vm.strings);
}

void vm_free(void) {
  Obj *object = vm.objects;

  while (object != NULL) {
    Obj *next = object->next;
    object_free(object);
    object = next;
  }

  table_free(&vm.strings);
}

static void stack_push(Value value) {
  *vm.stack_top = value;
  vm.stack_top += 1;
}

static Value stack_pop(void) {
  vm.stack_top -= 1;
  return *vm.stack_top;
}

static Value stack_peek(int idx) {
  return vm.stack_top[-1 - idx];
}

static void stack_reset(void) {}

static void vm_runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  int line = chunk_get_line(vm.chunk, (int)(vm.ip - vm.chunk->code - 1));
  fprintf(stderr, "[Line %d] in script\n", line);
  stack_reset();
}

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->consts.values[READ_BYTE()])

#define BINARY_OP(result_type, label, op)                                      \
  case label: {                                                                \
    if (!IS_NUMBER(stack_peek(0)) || !IS_NUMBER(stack_peek(1))) {              \
      vm_runtime_error("Operands must be numbers.");                           \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
                                                                               \
    double b = AS_NUMBER(stack_pop());                                         \
    double a = AS_NUMBER(stack_pop());                                         \
                                                                               \
    stack_push(result_type(a op b));                                           \
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
      BINARY_OP(NUMBER_VAL, OP_SUBTRACT, -);
      BINARY_OP(NUMBER_VAL, OP_MULTIPLY, *);
      BINARY_OP(NUMBER_VAL, OP_DIVIDE, /);

      BINARY_OP(BOOL_VAL, OP_LESSER, <);
      BINARY_OP(BOOL_VAL, OP_GREATER, >);

    case OP_RETURN:
      value_print(stack_pop());
      printf("\n");
      return INTERPRET_OK;

    case OP_LOAD:
      stack_push(READ_CONSTANT());
      break;

    case OP_NEGATE:
      if (!IS_NUMBER(stack_peek(0))) {
        vm_runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      stack_push(NUMBER_VAL(-AS_NUMBER(stack_pop())));
      break;

    case OP_ADD:
      if (IS_STRING(stack_peek(0)) && IS_STRING(stack_peek(1))) {
        ObjString *b = AS_STRING(stack_pop());
        ObjString *a = AS_STRING(stack_pop());

        int len = a->len + b->len;
        char *chars = MEM_ALLOC(char, len + 1);
        memcpy(chars, a->chars, a->len);
        memcpy(chars + a->len, b->chars, b->len);
        chars[len] = '\0';

        ObjString *result = string_create(chars, len);
        stack_push(OBJ_VAL(result));
      } else if (IS_NUMBER(stack_peek(0)) && IS_NUMBER(stack_peek(1))) {
        double b = AS_NUMBER(stack_pop());
        double a = AS_NUMBER(stack_pop());
        stack_push(NUMBER_VAL(a + b));
      } else {
        vm_runtime_error("Operands must be two strings or two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;

    case OP_NOT:
      stack_push(BOOL_VAL(value_is_falsey(stack_pop())));
      break;

    case OP_EQUAL: {
      Value b = stack_pop();
      Value a = stack_pop();
      stack_push(BOOL_VAL(value_is_equal(a, b)));
      break;
    }

    case OP_NIL:
      stack_push(NIL_VAL);
      break;

    case OP_TRUE:
      stack_push(BOOL_VAL(true));
      break;

    case OP_FALSE:
      stack_push(BOOL_VAL(false));
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
