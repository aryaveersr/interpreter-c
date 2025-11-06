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
#include <time.h>

VM vm;

static void reset_stack(void) {
  vm.stack_top = vm.stack;
  vm.frames_len = 0;
}

static Value native_clock(int arg_len, Value *args) {
  (void)arg_len;
  (void)args;
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

void vm_init(void) {
  vm.objects = NULL;
  vm.open_upvalues = NULL;

  table_init(&vm.strings);
  table_init(&vm.globals);
  reset_stack();

  define_native("clock", native_clock);
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

static void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top += 1;
}

static Value pop(void) {
  vm.stack_top -= 1;
  return *vm.stack_top;
}

static Value peek(int idx) {
  return vm.stack_top[-1 - idx];
}

static void runtime_error(const char *format, ...) {
  CallFrame *frame = &vm.frames[vm.frames_len - 1];
  Chunk *chunk = &frame->closure->function->chunk;

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  int line = chunk_get_line(chunk, (int)(frame->ip - chunk->code - 1));
  fprintf(stderr, "[Line %d] in script\n", line);

  printf("== Stack trace ==\n");
  for (int i = vm.frames_len - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;

    int instr = (int)(frame->ip - function->chunk.code - 1);
    fprintf(stderr, "[Line %d] in ", chunk_get_line(chunk, instr));

    if (function->name == NULL) {
      fprintf(stderr, "script.\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  reset_stack();
}

static bool call(ObjClosure *closure, int arg_len) {
  if (arg_len != closure->function->arity) {
    runtime_error("Expected %d arguments, but got %d.",
                  closure->function->arity, arg_len);
    return false;
  }

  if (vm.frames_len == FRAMES_MAX) {
    runtime_error("Call stack overflow.");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frames_len++];

  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stack_top - arg_len - 1;

  return true;
}

static bool call_value(Value callee, int arg_len) {
  if (IS_OBJ(callee)) {
    switch (OBJ_KIND(callee)) {
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), arg_len);

    case OBJ_NATIVE_FN: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(arg_len, vm.stack_top - arg_len);
      vm.stack_top -= arg_len + 1;
      push(result);
      return true;
    }

    default:
      runtime_error("Cannot call object of kind '%d'.", AS_OBJ(callee)->kind);
      break;
    }
  }

  runtime_error("Can only call functions and classes.");
  return false;
}

static ObjUpvalue *capture_upvalue(Value *local) {
  ObjUpvalue *prev = NULL;
  ObjUpvalue *upvalue = vm.open_upvalues;

  while (upvalue != NULL && upvalue->ptr > local) {
    prev = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->ptr == local) {
    return upvalue;
  }

  ObjUpvalue *created_upvalue = upvalue_new(local);
  created_upvalue->next = upvalue;

  if (prev == NULL) {
    vm.open_upvalues = created_upvalue;
  } else {
    prev->next = created_upvalue;
  }

  return created_upvalue;
}

static void close_upvalues(Value *last) {
  while (vm.open_upvalues != NULL && vm.open_upvalues->ptr >= last) {
    ObjUpvalue *upvalue = vm.open_upvalues;

    upvalue->closed = *upvalue->ptr;
    upvalue->ptr = &upvalue->closed;

    vm.open_upvalues = upvalue->next;
  }
}

void define_native(const char *name, NativeFn function) {
  push(OBJ_VAL(string_copy(name, (int)strlen(name))));
  push(OBJ_VAL(native_new(function)));
  table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

#define CHUNK() (&frame->closure->function->chunk)
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (CHUNK()->consts.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(result_type, label, op)                                      \
  case label: {                                                                \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtime_error("Operands must be numbers.");                              \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
                                                                               \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
                                                                               \
    push(result_type(a op b));                                                 \
    break;                                                                     \
  }

static InterpretResult run(void) {
  CallFrame *frame = &vm.frames[vm.frames_len - 1];

  while (true) {
#ifdef TRACE_VM
    printf("          ");

    for (Value *slot = vm.stack; slot < vm.stack_top; slot += 1) {
      printf("[");
      value_print(*slot);
      printf("] ");
    }

    printf("\n");
    chunk_print_instr(CHUNK(), (int)(frame->ip - CHUNK()->code));
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

    case OP_RETURN: {
      Value result = pop();
      close_upvalues(frame->slots);
      vm.frames_len--;

      if (vm.frames_len == 0) {
        pop();
        return INTERPRET_OK;
      }

      vm.stack_top = frame->slots;
      push(result);
      frame = &vm.frames[vm.frames_len - 1];
      break;
    }

    case OP_LOAD:
      push(READ_CONSTANT());
      break;

    case OP_NEGATE:
      if (!IS_NUMBER(peek(0))) {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;

    case OP_ADD:
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        ObjString *b = AS_STRING(pop());
        ObjString *a = AS_STRING(pop());

        int len = a->len + b->len;
        char *chars = MEM_ALLOC(char, len + 1);
        memcpy(chars, a->chars, a->len);
        memcpy(chars + a->len, b->chars, b->len);
        chars[len] = '\0';

        push(OBJ_VAL(string_new(chars, len)));
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else {
        runtime_error("Operands must be two strings or two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;

    case OP_NOT:
      push(BOOL_VAL(value_is_falsey(pop())));
      break;

    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(value_is_equal(a, b)));
      break;
    }

    case OP_NIL:
      push(NIL_VAL);
      break;

    case OP_TRUE:
      push(BOOL_VAL(true));
      break;

    case OP_FALSE:
      push(BOOL_VAL(false));
      break;

    case OP_PRINT:
      value_print(pop());
      printf("\n");
      break;

    case OP_POP:
      pop();
      break;

    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      table_set(&vm.globals, name, peek(0));
      pop();
      break;
    }

    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;

      if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }

      push(value);
      break;
    }

    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();

      if (table_set(&vm.globals, name, peek(0))) {
        table_remove(&vm.globals, name);
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }

      break;
    }

    case OP_GET_LOCAL:
      push(frame->slots[READ_BYTE()]);
      break;

    case OP_SET_LOCAL:
      frame->slots[READ_BYTE()] = peek(0);
      break;

    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }

    case OP_JUMP_BACK: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }

    case OP_JUMP_IF_TRUE: {
      uint16_t offset = READ_SHORT();
      if (!value_is_falsey(peek(0))) {
        frame->ip += offset;
      }
      break;
    }

    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (value_is_falsey(peek(0))) {
        frame->ip += offset;
      }
      break;
    }

    case OP_CALL: {
      int arg_len = READ_BYTE();
      if (!call_value(peek(arg_len), arg_len)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frames_len - 1];
      break;
    }

    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = closure_new(function);
      push(OBJ_VAL(closure));

      for (int i = 0; i < closure->upvalue_len; i++) {
        uint8_t is_local = READ_BYTE();
        uint8_t idx = READ_BYTE();

        if (is_local) {
          closure->upvalues[i] = capture_upvalue(frame->slots + idx);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[idx];
        }
      }
      break;
    }

    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->ptr);
      break;
    }

    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->ptr = peek(0);
      break;
    }

    case OP_CLOSE_UPVALUE:
      close_upvalues(vm.stack_top - 1);
      pop();
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

InterpretResult vm_interpret(ObjFunction *function) {
#ifdef TRACE_VM
  printf("\n== BEGIN VM ==\n\n");
#endif

  push(OBJ_VAL(function));
  ObjClosure *closure = closure_new(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}
