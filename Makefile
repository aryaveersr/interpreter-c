## Configuration START.
CC := clang

CFLAGS := -std=c18 -I./include
CFLAGS += -Wall -Wextra -Wpedantic -Wimplicit-fallthrough

PROFILE_FLAGS := -g -O3 -fno-omit-frame-pointer
DEBUG_FLAGS := -g -O0
RELEASE_FLAGS := -O3
## Configuration END.

SRCS := $(notdir $(wildcard src/*.c))
OBJS := $(addprefix bin/, $(SRCS:.c=.o))
DEPS := $(addprefix bin/, $(SRCS:.c=.d))
TARGET_FLAGS :=
TARGET_NAME :=

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

.PHONY: clean release debug profile

clean:
	@rm -rf ./bin/*

release: TARGET_FLAGS = $(RELEASE_FLAGS)
profile: TARGET_NAME = RELEASE
release: bin/lang.out

debug: TARGET_FLAGS = $(DEBUG_FLAGS)
profile: TARGET_NAME = DEBUG
debug: bin/lang.out

profile: TARGET_FLAGS = $(PROFILE_FLAGS)
profile: TARGET_NAME = PROFILE
profile: bin/lang.out

bin/%.dummy: src/%.c
	@include-what-you-use $(CFLAGS) -c $(patsubst %.dummy, %.c, $(subst bin/, src/, $@))

check-includes: $(addprefix bin/, $(SRCS:.c=.dummy))
