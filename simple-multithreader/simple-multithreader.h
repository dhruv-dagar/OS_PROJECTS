#ifndef SIMPLE_MULTITHREADER_H
#define SIMPLE_MULTITHREADER_H

#include <pthread.h>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>

// Struct for 1D thread arguments
struct ThreadArgs1D {
    int start;
    int end;
    std::function<void(int)> lambda;
};

// Struct for 2D thread arguments
struct ThreadArgs2D {
    int start1;
    int end1;
    int low2;
    int high2;
    std::function<void(int, int)> lambda;
};

// 1D thread function
void* threadFunction1D(void* args) {
    ThreadArgs1D* params = static_cast<ThreadArgs1D*>(args);
    for (int i = params->start; i < params->end; ++i) {
        params->lambda(i);
    }
    delete params; // Clean up dynamically allocated memory
    pthread_exit(nullptr);
}

// 2D thread function
void* threadFunction2D(void* args) {
    ThreadArgs2D* params = static_cast<ThreadArgs2D*>(args);
    for (int i = params->start1; i < params->end1; ++i) {
        for (int j = params->low2; j < params->high2; ++j) {
            params->lambda(i, j);
        }
    }
    delete params; // Clean up dynamically allocated memory
    pthread_exit(nullptr);
}

// Parallel for loop implementation for 1D range
void parallel_for(int low, int high, std::function<void(int)> lambda, int numThreads) {
    if (numThreads < 1) numThreads = 1; // Ensure at least one thread
    std::vector<pthread_t> threads(numThreads);
    int chunkSize = (high - low) / numThreads;
    int remainder = (high - low) % numThreads;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numThreads; ++i) {
        int start = low + i * chunkSize + std::min(i, remainder);
        int end = start + chunkSize + (i < remainder ? 1 : 0);

        ThreadArgs1D* args = new ThreadArgs1D{start, end, lambda};
        if (pthread_create(&threads[i], nullptr, threadFunction1D, args) != 0) {
            std::cerr << "Error creating thread\n";
            exit(EXIT_FAILURE);
        }
    }

    for (auto& thread : threads) {
        pthread_join(thread, nullptr);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "Execution time: " << elapsed.count() << " seconds\n";
}

// Parallel for loop implementation for 2D range
void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> lambda, int numThreads) {
    if (numThreads < 1) numThreads = 1; // Ensure at least one thread
    std::vector<pthread_t> threads(numThreads);
    int chunkSize = (high1 - low1) / numThreads;
    int remainder = (high1 - low1) % numThreads;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numThreads; ++i) {
        int start1 = low1 + i * chunkSize + std::min(i, remainder);
        int end1 = start1 + chunkSize + (i < remainder ? 1 : 0);

        ThreadArgs2D* args = new ThreadArgs2D{start1, end1, low2, high2, lambda};
        if (pthread_create(&threads[i], nullptr, threadFunction2D, args) != 0) {
            std::cerr << "Error creating thread\n";
            exit(EXIT_FAILURE);
        }
    }

    for (auto& thread : threads) {
        pthread_join(thread, nullptr);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "Execution time: " << elapsed.count() << " seconds\n";
}

#endif // SIMPLE_MULTITHREADER_H
