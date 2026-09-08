// fib.c — minimal 32-bit ELF test that returns a value to the loader
// No glibc, no system calls; _start() returns control to loader

int fib(int n) {
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

// Entry point: computes fib(10) and returns result (should be 55)
int _start() {
    int result = fib(10);
    return result;   // returns to loader
}
