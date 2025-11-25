## Configuration START.
CC := clang

CFLAGS := -std=c18 -I./include
CFLAGS += -Wall -Wextra -Wpedantic -Wimplicit-fallthrough

PROFILE_FLAGS := -g -O3 -fno-omit-frame-pointer
DEBUG_FLAGS   := -g -O0
RELEASE_FLAGS := -O3
## Configuration END.

## Files.
SRCS := $(notdir $(wildcard src/*.c))
OBJS := $(addprefix bin/, $(SRCS:.c=.o))
DEPS := $(wildcard bin/*.d)

TARGET_NAME  :=
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
	@echo "Built for target: '$(TARGET_NAME)' at \033[4;37m$@\033[0m"

## Phony targets.
.PHONY: clean release debug profile check-iwyu check-clang-tidy

# Clean the build directory.
clean:
	@rm -rf ./bin/*

# Release build.
release: TARGET_FLAGS = $(RELEASE_FLAGS)
release: TARGET_NAME = RELEASE
release: bin/lang.out

# Debug build.
debug: TARGET_FLAGS = $(DEBUG_FLAGS)
debug: TARGET_NAME = DEBUG
debug: bin/lang.out

# Profiling build.
profile: TARGET_FLAGS = $(PROFILE_FLAGS)
profile: TARGET_NAME = PROFILE
profile: bin/lang.out

## Check includes.
bin/%.iwyu: src/%.c
	@include-what-you-use $(CFLAGS) -c $(patsubst %.iwyu, %.c, $(subst bin/, src/, $@))

check-iwyu: $(addprefix bin/, $(SRCS:.c=.iwyu))

# Check clang-tidy.
check-clang-tidy:
	@bear -- make debug -B > /dev/null
	@clang-tidy src/*.c include/*.h
