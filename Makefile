# Compiler
CC = $(CROSS_COMPILE)gcc

# Compiler flags
CFLAGS = -g

# Source files
LIB_SRCS = src/posix_shmem.c

TEST_SRCS ?= test/test_queue.c

# Output executable
TARGET ?= w

# Output directory (default value)
OUTPUT_DIR ?= ./build

all: $(OUTPUT_DIR) $(OUTPUT_DIR)/$(TARGET)

# Ensure the output directory exists
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Default rule

# Rule for building the target
$(OUTPUT_DIR)/$(TARGET): $(LIB_SRCS) $(TEST_SRCS) 
	$(CC) -o $(OUTPUT_DIR)/$(TARGET) $(LIB_SRCS) $(TEST_SRCS) $(CFLAGS)

# Clean rule
clean:
	rm -f $(OUTPUT_DIR)/*

# Phony targets
.PHONY: all clean
