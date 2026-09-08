# Simple Smart Loader — Architecture

## Overview

Simple Smart Loader is a user-space ELF32 loader for Linux. Instead of loading all executable segments before execution, it relies on page faults to load individual pages on demand.

```text
                    +----------------+
                    |    launch.c    |
                    | validate ELF   |
                    +-------+--------+
                            |
                            v
                    +----------------+
                    |    loader.c    |
                    | parse ELF32    |
                    | install SIGSEGV|
                    +-------+--------+
                            |
                     jump to e_entry
                            |
                            v
                    +----------------+
                    |   ELF program  |
                    |    _start()    |
                    +-------+--------+
                            |
                     access unmapped
                         virtual page
                            |
                            v
                    +----------------+
                    | SIGSEGV handler|
                    +-------+--------+
                            |
              +-------------+-------------+
              |                           |
              v                           v
       Find PT_LOAD segment        Align fault to 4 KiB
              |                           |
              +-------------+-------------+
                            |
                            v
                    +----------------+
                    |     mmap()     |
                    | allocate page  |
                    +-------+--------+
                            |
                            v
                    +----------------+
                    | Copy file data |
                    | / zero BSS     |
                    +-------+--------+
                            |
                            v
                    +----------------+
                    |   mprotect()   |
                    | ELF R/W/X bits |
                    +-------+--------+
                            |
                            v
                     resume execution
```

## Execution flow

### 1. ELF validation

`launch.c` receives the executable path, opens the target through the loader, and validates the ELF magic before attempting execution.

### 2. ELF parsing

`loader.c` reads the ELF image and obtains:

- `Elf32_Ehdr` — ELF header
- `Elf32_Phdr` — program headers
- `e_entry` — executable entry point
- `PT_LOAD` segments — regions that need to become part of the process address space

### 3. Demand paging

The loader does not eagerly map every segment page. It installs a `SIGSEGV` handler and jumps to the ELF entry point. Access to an unmapped executable page triggers the handler.

### 4. Page-fault handling

For each fault, the handler:

1. Reads the fault address from `siginfo_t`.
2. Aligns it down to a 4096-byte page boundary.
3. Locates the `PT_LOAD` segment containing the address.
4. Maps exactly one page at the fault address using `mmap(..., MAP_FIXED, ...)`.
5. Copies the file-backed portion of the page from the ELF image.
6. Zero-fills the portion represented by BSS (`p_memsz > p_filesz`).
7. Applies the segment's `PF_R`, `PF_W`, and `PF_X` permissions with `mprotect()`.
8. Returns from the signal handler so execution can continue.

### 5. Memory accounting

The loader records:

- Number of page faults
- Number of pages allocated
- Total virtual memory allocated in 4 KiB pages
- Bytes copied from the executable image
- Approximate internal fragmentation

## Why this is interesting from an OS perspective

The project demonstrates several mechanisms normally handled by an operating system's executable loader and virtual-memory subsystem:

- ELF executable format parsing
- Virtual address spaces
- Page alignment
- Demand paging
- Page-fault-driven memory population
- File-backed memory
- BSS zero initialization
- Memory protection
- Low-level process startup

## Design trade-offs

This implementation is intentionally educational. It keeps the complete ELF file in a userspace buffer and performs page population from that buffer. This makes the paging logic easy to observe, but differs from a production kernel loader, which would normally coordinate virtual memory, file mappings, permissions, process state, and fault handling inside the kernel.

The SIGSEGV handler also performs operations such as allocation and formatted output that are not generally appropriate for production signal handlers. They are retained here to keep the educational implementation observable and straightforward.
