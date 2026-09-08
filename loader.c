// loader.c - fixed version for page-by-page lazy loading (ELF32)
// Replace your existing loader.c with this file.

#include "loader.h"
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/param.h>

// constants
#define PAGE_SIZE 4096

// ELF structures
Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;

// file buffer & fd
int fd = -1;
void *file_buf = NULL;
off_t file_size = 0;

// statistics
int no_page_fault = 0;
int no_page_allocation = 0;
size_t total_mem_allocated = 0; // in bytes (pages allocated * PAGE_SIZE)
size_t total_mem_used = 0;      // bytes actually copied from file (sum of copied bytes)

// allocated pages bookkeeping (simple dynamic array)
void **allocated_pages = NULL;
size_t allocated_pages_count = 0;
size_t allocated_pages_capacity = 0;

// helpers for bookkeeping
static void add_allocated_page(void *addr) {
    if (allocated_pages_count == allocated_pages_capacity) {
        size_t newcap = allocated_pages_capacity ? allocated_pages_capacity * 2 : 64;
        void **tmp = realloc(allocated_pages, newcap * sizeof(void *));
        if (!tmp) return; // if realloc fails we still try to continue (best-effort)
        allocated_pages = tmp;
        allocated_pages_capacity = newcap;
    }
    allocated_pages[allocated_pages_count++] = addr;
}

static int is_page_allocated(void *addr) {
    for (size_t i = 0; i < allocated_pages_count; ++i) {
        if (allocated_pages[i] == addr) return 1;
    }
    return 0;
}

// forward declaration
void loader_cleanup();

// Helper: read the entire file into file_buf
static int read_entire_file(void) {
    if (fd < 0) return 0;
    file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) return 0;
    if (lseek(fd, 0, SEEK_SET) == -1) return 0;
    file_buf = malloc((size_t)file_size);
    if (!file_buf) return 0;

    ssize_t total = 0;
    while (total < file_size) {
        ssize_t r = read(fd, (char *)file_buf + total, file_size - total);
        if (r < 0) {
            if (errno == EINTR) continue;
            free(file_buf);
            file_buf = NULL;
            return 0;
        }
        if (r == 0) break;
        total += r;
    }
    return (total == file_size) ? 1 : 0;
}

// SIGSEGV handler
void segfault_handler(int signo, siginfo_t *info, void *context) {
    (void)signo;
    (void)context;
    no_page_fault++;

    void *fault_addr = info->si_addr;
    // page-align the fault address to the page boundary
    uintptr_t fault_page_addr_u = (uintptr_t)fault_addr & ~(PAGE_SIZE - 1);
    void *fault_page_addr = (void *)fault_page_addr_u;

    // if this page is already allocated according to our bookkeeping, avoid infinite loop
    if (is_page_allocated(fault_page_addr)) {
        // Already tried allocating this page; print debugging info and abort to avoid tight loop.
        fprintf(stderr, "Repeated page fault at already-allocated page %p — aborting to avoid infinite loop\n", fault_page_addr);
        loader_cleanup();
        _exit(1);
    }

    // find the program header (PT_LOAD) that contains this fault address
    Elf32_Phdr *target_phdr = NULL;
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        uintptr_t seg_vaddr = (uintptr_t)phdr[i].p_vaddr;
        uintptr_t seg_vend  = seg_vaddr + phdr[i].p_memsz;
        if ((uintptr_t)fault_addr >= seg_vaddr && (uintptr_t)fault_addr < seg_vend) {
            target_phdr = &phdr[i];
            break;
        }
    }

    if (!target_phdr) {
        // Not inside a PT_LOAD segment: re-raise default handler (or abort)
        fprintf(stderr, "Segfault at %p not in any PT_LOAD segment — cannot handle\n", fault_addr);
        loader_cleanup();
        _exit(1);
    }

    // compute page offset of this page inside the segment
    uintptr_t seg_base = (uintptr_t)target_phdr->p_vaddr;
    uintptr_t page_offset_in_seg = fault_page_addr_u - seg_base; // may be 0, PAGE_SIZE, etc.

    // check limits: only allocate if this page falls within p_memsz
    if (page_offset_in_seg >= (uintptr_t)target_phdr->p_memsz) {
        fprintf(stderr, "Fault page %p is beyond segment memsz — cannot handle\n", fault_page_addr);
        loader_cleanup();
        _exit(1);
    }

    // Map exactly one page at the fault page address
    void *mapped = mmap(fault_page_addr, PAGE_SIZE,
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap failed in segfault handler");
        loader_cleanup();
        _exit(1);
    }

    // Bookkeeping
    add_allocated_page(fault_page_addr);
    no_page_allocation++;
    total_mem_allocated += PAGE_SIZE;

    // Determine what portion of this page is backed by the file (p_filesz) and what must be zeroed (bss)
    uintptr_t file_backed_bytes = 0;
    if (page_offset_in_seg < (uintptr_t)target_phdr->p_filesz) {
        // bytes available in file for this page:
        uintptr_t bytes_remaining_in_file = (uintptr_t)target_phdr->p_filesz - page_offset_in_seg;
        file_backed_bytes = (bytes_remaining_in_file >= PAGE_SIZE) ? PAGE_SIZE : bytes_remaining_in_file;
    } else {
        file_backed_bytes = 0;
    }

    // copy file-backed bytes (if any) from file_buf
    if (file_backed_bytes > 0) {
        size_t copy_src_offset = (size_t)target_phdr->p_offset + (size_t)page_offset_in_seg;
        // bounds check
        if (copy_src_offset + file_backed_bytes <= (size_t)file_size) {
            memcpy(mapped, (char *)file_buf + copy_src_offset, file_backed_bytes);
            total_mem_used += file_backed_bytes;
        } else {
            // safety: if file doesn't contain expected bytes, zero-fill and continue
            size_t safe_bytes = 0;
            if (copy_src_offset < (size_t)file_size) safe_bytes = (size_t)file_size - copy_src_offset;
            if (safe_bytes > 0) {
                memcpy(mapped, (char *)file_buf + copy_src_offset, safe_bytes);
                total_mem_used += safe_bytes;
            }
            if (file_backed_bytes > safe_bytes) {
                memset((char *)mapped + safe_bytes, 0, file_backed_bytes - safe_bytes);
            }
        }
    }

    // Zero the remainder of the page if it belongs to p_memsz but not p_filesz (bss)
    if (file_backed_bytes < PAGE_SIZE) {
        // compute how many bytes should be zeroed on this page within p_memsz
        uintptr_t page_mem_bytes = PAGE_SIZE;
        // but if this page extends beyond p_memsz, only zero the part within p_memsz
        uintptr_t bytes_in_seg_page = PAGE_SIZE;
        uintptr_t seg_remaining = (uintptr_t)target_phdr->p_memsz - page_offset_in_seg;
        if (seg_remaining < bytes_in_seg_page) bytes_in_seg_page = seg_remaining;
        // bytes to zero = page_mem_bytes (or bytes_in_seg_page) - file_backed_bytes
        if (bytes_in_seg_page > file_backed_bytes) {
            size_t to_zero = (size_t)(bytes_in_seg_page - file_backed_bytes);
            memset((char *)mapped + file_backed_bytes, 0, to_zero);
        }
        // if this page extends beyond the segment, we leave rest unmapped (but we mapped full page via MAP_FIXED)
        // It's acceptable to zero beyond p_memsz for simplicity (the page is accessible but data is zero).
    }

    // Remap page protections based on segment flags: phdr->p_flags (PF_R/W/X)
    int prot = 0;
    if (target_phdr->p_flags & PF_R) prot |= PROT_READ;
    if (target_phdr->p_flags & PF_W) prot |= PROT_WRITE;
    if (target_phdr->p_flags & PF_X) prot |= PROT_EXEC;
    if (mprotect(fault_page_addr, PAGE_SIZE, prot) == -1) {
        // mprotect may fail; not fatal, but print a warning
        // (we previously mapped RWX for copy reasons)
        perror("mprotect warning");
    }

    // done handling this page fault
    return;
}

// loader cleanup
void loader_cleanup() {
    if (file_buf) {
        free(file_buf);
        file_buf = NULL;
    }
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    if (allocated_pages) {
        free(allocated_pages);
        allocated_pages = NULL;
        allocated_pages_count = 0;
        allocated_pages_capacity = 0;
    }
}

// load and run ELF
void load_and_run_elf(const char *exe) {
    // open ELF
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("open ELF");
        return;
    }

    // read entire file
    if (!read_entire_file()) {
        fprintf(stderr, "Failed to read ELF file into memory\n");
        loader_cleanup();
        return;
    }

    // set up ELF pointers in memory buffer
    if ((size_t)file_size < sizeof(Elf32_Ehdr)) {
        fprintf(stderr, "File too small to be ELF\n");
        loader_cleanup();
        return;
    }
    ehdr = (Elf32_Ehdr *)file_buf;
    if (!(ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
          ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
          ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
          ehdr->e_ident[EI_MAG3] == ELFMAG3)) {
        fprintf(stderr, "Not a valid ELF file\n");
        loader_cleanup();
        return;
    }
    // program headers pointer inside file_buf
    phdr = (Elf32_Phdr *)((char *)file_buf + ehdr->e_phoff);

    // register SIGSEGV handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sa.sa_sigaction = segfault_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        loader_cleanup();
        return;
    }

    // Jump to entrypoint (cast to function pointer). The loader relies on SIGSEGV handler to map pages lazily.
    int (*_start)() = (int (*)())(intptr_t)ehdr->e_entry;
    int result = 0;
    // call start; if it returns, capture return value
    result = _start();

    // After execution, print statistics and cleanup
    printf("User _start return value = %d\n", result);
    printf("No of page faults: %d\n", no_page_fault);
    printf("Total memory allocated: %zu\n", total_mem_allocated);
    printf("Total memory used (bytes copied from file): %zu\n", total_mem_used);
    printf("Internal fragmentation: %.2f KB\n", (double)(total_mem_allocated - total_mem_used) / 1024.0);
    printf("Total page allocation: %d\n", no_page_allocation);

    loader_cleanup();
}
