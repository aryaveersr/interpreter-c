## Configuration START.
CC := clang

CFLAGS := -std=c18 -I./include
CFLAGS += -Wall -Wextra -Wpedantic -Wimplicit-fallthrough

DEBUG_FLAGS := -g -Og
RELEASE_FLAGS := -O3
## Configuration END.

SRCS := $(notdir $(wildcard src/*.c))
OBJS := $(addprefix bin/, $(SRCS:.c=.o))
DEPS := $(addprefix bin/, $(SRCS:.c=.d))
TARGET_FLAGS :=

-include $(DEPS)

bin/%.o: src/%.c
	@echo -n "Building \033[4;37m$<\033[0m\n"
	@$(CC) $(CFLAGS) $(TARGET_FLAGS) -MMD -MP -c $(patsubst %.o, %.c, $(subst bin/, src/, $@)) -o $@
	@echo "\033[1;92mDONE\033[0m"

bin/lang.out: $(OBJS)
	@echo -n "Linking\n"
	@$(CC) $(CFLAGS) $(TARGET_FLAGS) $^ -o $@
	@echo "\033[1;92mDONE\033[0m"
	@echo "Release built at \033[4;37m$@\033[0m"

bin/lang-dbg.out: $(OBJS)
	@echo -n "Linking\n"
	@$(CC) $(CFLAGS) $(TARGET_FLAGS) $^ -o $@
	@echo "\033[1;92mDONE\033[0m"
	@echo "Debug built at \033[4;37m$@\033[0m"

.PHONY: clean release debug

clean:
	@rm -rf ./bin/*

release: TARGET_FLAGS = $(RELEASE_FLAGS)
release: bin/lang.out

debug: TARGET_FLAGS = $(DEBUG_FLAGS)
debug: bin/lang-dbg.out

bin/%.dummy: src/%.c
	@include-what-you-use $(CFLAGS) -c $(patsubst %.dummy, %.c, $(subst bin/, src/, $@))

check-includes: $(addprefix bin/, $(SRCS:.c=.dummy))
