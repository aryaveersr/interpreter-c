#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
#include <stdbool.h>

bool compiler_compile(Chunk *chunk);

#endif
