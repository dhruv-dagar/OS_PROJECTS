CC = gcc
CFLAGS = -Wall -m32
TESTFLAGS = -Wall -m32 -no-pie -nostdlib

BIN_DIR = bin

# Targets
all: $(BIN_DIR)/loader test/fib

# Build loader
$(BIN_DIR)/loader: loader/loader.c loader/loader.h
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/loader loader/loader.c

# Build test program (fib)
test/fib: test/fib.c
	$(CC) $(TESTFLAGS) -o test/fib test/fib.c

# Cleanup
clean:
	rm -rf $(BIN_DIR)/*
	rm -f test/fib
