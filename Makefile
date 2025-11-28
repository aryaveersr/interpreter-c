## Configuration START.
CC := clang

CFLAGS := -Iinclude
CFLAGS += -Wall -Wextra -Wpedantic

PROFILE_FLAGS := -g -O3 -fno-omit-frame-pointer
DEBUG_FLAGS   := -g -O0
RELEASE_FLAGS := -O3
## Configuration END.

## Files.
SRCS := $(notdir $(wildcard src/*.c))
OBJS := $(addprefix bin/objs/, $(SRCS:.c=.o))
DEPS := $(wildcard bin/deps/*.d)

TARGET_FLAGS :=

-include $(DEPS)

bin/objs/%.o: src/%.c
	$(CC) $(CFLAGS) $(TARGET_FLAGS) -MP -c $< -o $@ -MMD -MF $(patsubst %.o, %.d, $(subst bin/objs/, bin/deps/, $@))

bin/lang.out: $(OBJS)
	$(CC) $(CFLAGS) $(TARGET_FLAGS) $^ -o $@

## Phony targets.
.PHONY: clean release debug profile check-iwyu check-clang-tidy

# Clean the build directory.
clean:
	rm -rf ./bin/objs/*
	rm -rf ./bin/deps/*
	rm -rf ./bin/lang.out

# Release build.
release: TARGET_FLAGS = $(RELEASE_FLAGS)
release: bin/lang.out

# Debug build.
debug: TARGET_FLAGS = $(DEBUG_FLAGS)
debug: bin/lang.out

# Profiling build.
profile: TARGET_FLAGS = $(PROFILE_FLAGS)
profile: bin/lang.out

## Check includes.
bin/%.iwyu: src/%.c
	@include-what-you-use $(CFLAGS) -c $(patsubst %.iwyu, %.c, $(subst bin/, src/, $@))

check-iwyu: $(addprefix bin/, $(SRCS:.c=.iwyu))

# Check clang-tidy.
check-clang-tidy:
	@bear -- make debug -B > /dev/null
	@clang-tidy src/*.c include/*.h
