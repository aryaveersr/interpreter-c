#ifndef COMPILER_H
#define COMPILER_H

#include "object.h"

#define DUMP_CODE

ObjFunction *compiler_compile(void);
void mark_compiler_roots(void);

#endif
