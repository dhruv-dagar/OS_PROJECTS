#include "dummy_main.h"
#include <stdio.h>
#include <unistd.h>  // Add this for getpid()

int dummy_main(int argc, char **argv) {
    printf("Dummy B started (PID: %d)\n", getpid());
    for(int i = 1; i <= 8; i++) {
        printf("Dummy B is running. Iteration %d\n", i);
        // Longer busy wait
        for(int j = 0; j < 400000000; j++);
    }
    printf("Dummy B finished\n");
    return 0;
}