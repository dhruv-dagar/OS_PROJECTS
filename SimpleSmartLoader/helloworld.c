// helloworld.c — minimal 32-bit ELF test for SimpleSmartLoader
// Uses raw syscalls (no glibc), writes "Hello, World!\n" to stdout
// and returns to the loader for stats printing.

static const char msg[] = "Hello, World!\n";

int _start() {
    // write(1, msg, sizeof(msg)-1)
    __asm__ volatile (
        "movl $4, %%eax\n\t"         // syscall number 4 = sys_write
        "movl $1, %%ebx\n\t"         // file descriptor 1 = stdout
        "movl %0, %%ecx\n\t"         // pointer to message
        "movl %1, %%edx\n\t"         // message length
        "int $0x80\n\t"              // make syscall
        :
        : "r"(msg), "r"(sizeof(msg) - 1)
        : "%eax", "%ebx", "%ecx", "%edx"
    );

    // Return a code to the loader (not exit syscall)
    return 0x7F; // just a test return value
}
