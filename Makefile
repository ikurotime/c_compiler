CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS =

# add -g -O0 for debugging, or -O2 for speed
BUILD_TYPE ?= debug
ifeq ($(BUILD_TYPE), debug)
    CFLAGS += -g -O0
else
    CFLAGS += -O2
endif

SRC_DIR = src
BUILD   = build
BIN     = $(BUILD)/compiler

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD)/%.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean test

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

# test: compile each .c file in tests/ with the compiler and run it
# expects the compiler to eventually emit executables; for now just a placeholder
test: $(BIN)
	@echo "no tests yet — add programs to tests/ once codegen works"

clean:
	rm -rf $(BUILD)

-include $(DEPS)
