#include "dummy_main.h"
#include <stdio.h>
#include <unistd.h>  // Add this for getpid()

int dummy_main(int argc, char **argv) {
    printf("Dummy A started (PID: %d)\n", getpid());
    for(int i = 1; i <= 10; i++) {
        printf("Dummy A is running. Iteration %d\n", i);
        // Longer busy wait
        for(int j = 0; j < 500000000; j++);
    }
    printf("Dummy A finished\n");
    return 0;
}