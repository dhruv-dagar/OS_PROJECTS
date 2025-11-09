# Makefile for 32-bit SimpleSmartLoader

CC = gcc
CFLAGS = -Wall -Wextra -m32 -fPIC
LDFLAGS = -shared -m32
TARGET = lib_simpleloader.so

all: $(TARGET) ../launch ../fib

$(TARGET): loader.c loader.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) loader.c

../launch: ../launch.c loader.h
	gcc -m32 -I. -Wl,-rpath,'$$ORIGIN/loader' -o ../launch ../launch.c -L. -l_simpleloader -ldl

../fib: ../fib.c
	$(CC) -m32 -nostdlib -o ../fib ../fib.c

clean:
	rm -f $(TARGET) ../launch ../fib
