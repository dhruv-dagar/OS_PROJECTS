# Simple Multithreader

A lightweight C++ multithreading library built with POSIX threads (`pthreads`). It provides reusable `parallel_for` primitives for 1D and 2D workloads and demonstrates explicit thread creation, work partitioning, synchronization, and parallel performance measurement.

## Highlights

- 1D and 2D `parallel_for` abstractions
- POSIX `pthread_create()` / `pthread_join()` based execution
- Automatic chunk distribution across worker threads
- Handles uneven work by distributing remainder iterations
- Measures parallel execution time with `std::chrono`
- Includes vector and matrix examples

## Build

```bash
make
```

## Run

```bash
./vector
./matrix
```

## Architecture

The library divides an iteration space into chunks, creates worker threads, and assigns each worker a disjoint range of iterations. Workers invoke the user-provided function over their assigned range. The caller waits for all workers with `pthread_join()` before returning.

For 2D workloads, the same approach is applied to matrix regions so independent work can execute concurrently.

## OS concepts demonstrated

- POSIX threads
- Thread creation and joining
- Work partitioning
- Concurrency and synchronization
- Parallel speedup measurement
- Shared-memory parallelism

## Project structure

```text
simple-multithreader/
├── README.md
├── Makefile
├── .gitignore
├── simple-multithreader.h
├── vector.cpp
├── matrix.cpp
└── IMPLEMENTATION OF SIMPLE MULTITHREADER.docx
```

## Author

Dhruv Dagar
