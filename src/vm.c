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
  table_init(&vm.globals);
}

void vm_free(void) {
  Obj *object = vm.objects;

  while (object != NULL) {
    Obj *next = object->next;
    object_free(object);
    object = next;
  }

  table_free(&vm.strings);
  table_free(&vm.globals);
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

static void runtime_error(const char *format, ...) {
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
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_CONSTANT() (vm.chunk->consts.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(result_type, label, op)                                      \
  case label: {                                                                \
    if (!IS_NUMBER(stack_peek(0)) || !IS_NUMBER(stack_peek(1))) {              \
      runtime_error("Operands must be numbers.");                              \
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
      return INTERPRET_OK;

    case OP_LOAD:
      stack_push(READ_CONSTANT());
      break;

    case OP_NEGATE:
      if (!IS_NUMBER(stack_peek(0))) {
        runtime_error("Operand must be a number.");
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
        runtime_error("Operands must be two strings or two numbers.");
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

    case OP_PRINT:
      value_print(stack_pop());
      printf("\n");
      break;

    case OP_POP:
      stack_pop();
      break;

    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      table_set(&vm.globals, name, stack_peek(0));
      stack_pop();
      break;
    }

    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;

      if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }

      stack_push(value);
      break;
    }

    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();

      if (table_set(&vm.globals, name, stack_peek(0))) {
        table_remove(&vm.globals, name);
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }

      break;
    }

    case OP_GET_LOCAL:
      stack_push(vm.stack[READ_BYTE()]);
      break;

    case OP_SET_LOCAL:
      vm.stack[READ_BYTE()] = stack_peek(0);
      break;

    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      vm.ip += offset;
      break;
    }

    case OP_JUMP_BACK: {
      uint16_t offset = READ_SHORT();
      vm.ip -= offset;
      break;
    }

    case OP_JUMP_IF_TRUE: {
      uint16_t offset = READ_SHORT();
      if (!value_is_falsey(stack_peek(0))) {
        vm.ip += offset;
      }
      break;
    }

    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (value_is_falsey(stack_peek(0))) {
        vm.ip += offset;
      }
      break;
    }

    default:
      printf("Unknown opcode: '%d'.\n", instr);
      return INTERPRET_RUNTIME_ERROR;
    }
#pragma clang diagnostic pop
  }
}

#undef READ_BYTE
#undef READ_CONSTANT
