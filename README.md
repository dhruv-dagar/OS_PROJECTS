# Process Scheduler

A Linux process-scheduling simulator in C that demonstrates CPU scheduling, priorities, preemption, signals, shared memory, and process synchronization.

## Highlights

- Priority-based ready queue with configurable CPU count and time slice
- Uses `fork()` to create workload processes
- Uses `SIGSTOP` / `SIGCONT` for process preemption and dispatch
- Handles `SIGCHLD` for child lifecycle events
- Uses System V shared memory for scheduler state
- Uses process-shared POSIX semaphores for synchronization
- Maintains scheduling history and job state
- Includes a shell interface for submitting executable jobs

## Build

```bash
make
```

## Run

```bash
./simpleshell <NCPU> <TSLICE_MS>
```

Then submit jobs using:

```text
submit <executable> [priority]
exit
```

## Architecture

`simpleshell.c` provides the user-facing command interface and submits processes. `simplescheduler.c` manages the ready queue, dispatches processes across CPUs, enforces time slices, and coordinates process state using Linux signals and shared synchronization primitives.

## OS concepts demonstrated

- CPU scheduling and preemption
- Process creation and lifecycle management
- Signals and asynchronous events
- Inter-process communication
- Shared memory
- Semaphores and race-condition prevention
- Ready queues and priority scheduling

## Project structure

```text
process-scheduler/
├── README.md
├── Makefile
├── .gitignore
├── simplescheduler.c
├── simpleshell.c
├── dummy_a.c
├── dummy_b.c
└── dummy_main.h
```

## Author

Dhruv Dagar
