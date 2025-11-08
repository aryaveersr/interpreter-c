#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "object.h"
#include <stdbool.h>
#include <stdint.h>

#define DUMP_CODE

typedef struct {
  Token name;
  int depth;
  bool is_captured;
} Local;

typedef enum {
  TARGET_SCRIPT,
  TARGET_FUNCTION,
  TARGET_METHOD,
  TARGET_CONSTRUCTOR,
} TargetKind;

struct Compiler {
  ObjFunction *function;
  TargetKind kind;

  Local locals[UINT8_MAX + 1];
  int len;
  int depth;

  Upvalue upvalues[UINT8_MAX + 1];

  struct Compiler *parent;
};

typedef struct Compiler Compiler;

ObjFunction *compiler_compile(void);
Compiler *compiler_current(void);

#endif
