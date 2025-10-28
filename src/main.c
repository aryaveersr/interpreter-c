#include "lexer.h"
#include "vm.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static InterpretResult run_source(const char *source) {
  lexer_init(source);

  int line = -1;

  while (true) {
    Token token = lexer_next();

    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    } else {
      printf("   | ");
    }

    printf("%2d '%.*s'\n", token.kind, token.len, token.start);

    if (token.kind == TOKEN_EOF) {
      break;
    }
  }

  return INTERPRET_OK;
}

static void repl(void) {
  char line[1024];

  while (true) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin) || strncmp(line, "quit", 4) == 0 ||
        strncmp(line, "exit", 4) == 0) {
      printf("\n");
      break;
    }

    run_source(line);
  }
}

static char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    fprintf(stderr, "Could not open file: '%s'.\n", path);
    exit(EXIT_FAILURE);
  }

  if (fseek(file, 0L, SEEK_END) == -1) {
    exit(EXIT_FAILURE);
  }

  long file_size = ftell(file);

  if (file_size == -1 || fseek(file, 0L, SEEK_SET) == -1) {
    exit(EXIT_FAILURE);
  }

  char *buffer = (char *)malloc(file_size + 1);

  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read file: '%s'.\n", path);
    exit(EXIT_FAILURE);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);

  if (bytes_read < (size_t)file_size) {
    fprintf(stderr, "Could not read file: '%s'.\n", path);
    exit(EXIT_FAILURE);
  }

  buffer[file_size] = '\0';

  fclose(file);
  return buffer;
}

static void run_file(const char *path) {
  char *source = read_file(path);
  InterpretResult result = run_source(source);
  free(source);

  if (result != INTERPRET_OK) {
    fprintf(stderr, "Interpreter returned error code: %d.", result);
    exit(EXIT_FAILURE);
  }
}

int main(int argc, const char *argv[]) {
  vm_init();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  } else {
    fprintf(stderr, "Usage: wee [path]\n");
    exit(EXIT_FAILURE);
  }

  vm_free();
  return 0;
}
