#include <stddef.h>
#include <sys/mman.h>

#include "contracts.h"


int prime_count(int a, int b) {
    if (b < 2) {
        return 0;
    }
    if (a < 2) {
        a = 2;
    }
    if (a > b) {
        return 0;
    }

    const size_t last = (size_t)b;
    const size_t size = last + 1;

    unsigned char *composite = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (composite == MAP_FAILED) {
        return -1;
    }
    for (size_t i = 2; i * i <= last; i++) {
        if (composite[i]) {
            continue;
        }
        for (size_t j = i * i; j <= last; j += i) {
            composite[j] = 1;
        }
    }

    int count = 0;
    for (size_t n = (size_t)a; n <= last; n++) {
        if (!composite[n]) {
            count++;
        }
    }

    munmap(composite, size);
    return count;
}

float e(int x) {
    if (x < 0) {
        return 0.0f;
    }

    double term = 1.0;
    double sum = 1.0;
    for (int n = 1; n <= x; n++) {
        term /= (double)n;
        if (term == 0.0) {
            break;
        }
        sum += term;
    }
    return (float)sum;
}
