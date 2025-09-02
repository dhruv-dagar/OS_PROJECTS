#include "loader.h"
#include <sys/stat.h>

static int fd;
static void *file_mem = NULL;
static size_t file_size = 0;

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;

void loader_cleanup() {
    if (file_mem) {
        munmap(file_mem, file_size);
        file_mem = NULL;
    }
    if (fd > 0) {
        close(fd);
    }
}

void load_and_run_elf(char **exe) {
    // exe[1] contains ELF filename
    fd = open(exe[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // Get file size
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
    perror("lseek");
    exit(1);
    }
    file_size = (size_t) size;
    lseek(fd, 0, SEEK_SET); // rewind to start



    // Map file into memory
    file_mem = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Parse ELF header + program headers
    ehdr = (Elf32_Ehdr *)file_mem;
    phdr = (Elf32_Phdr *)((char*)file_mem + ehdr->e_phoff);

    // Walk program headers and load PT_LOAD segments
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            void *segment = mmap((void*)phdr[i].p_vaddr,
                                 phdr[i].p_memsz,
                                 PROT_READ | PROT_WRITE | PROT_EXEC,
                                 MAP_PRIVATE | MAP_ANONYMOUS,
                                 -1, 0);
            if (segment == MAP_FAILED) {
                perror("mmap segment");
                exit(1);
            }
            memcpy(segment,
                   (char*)file_mem + phdr[i].p_offset,
                   phdr[i].p_filesz);
        }
    }

    // Jump to entry point
    void *entry_addr = (void*)ehdr->e_entry;
    int (*_start)() = (int (*)())entry_addr;
    int result = _start();
    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv);
    loader_cleanup();

    return 0;
}
