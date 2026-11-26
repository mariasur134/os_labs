#include "contracts.h"

int prime_count(int a, int b) {
    if (a < 2) {
        a = 2;
    }

    int count = 0;
    for (long long n = a; n <= b; n++) {
        int is_prime = 1;
        for (long long d = 2; d < n; d++) {
            if (n % d == 0) {
                is_prime = 0;
                break;
            }
        }
        count += is_prime;
    }
    return count;
}


float e(int x) {
    if (x <= 0) {
        return 0.0f;
    }
    double base = 1.0 + 1.0 / (double)x;
    double result = 1.0;
    unsigned int power = (unsigned int)x;
    while (power > 0) {
        if (power & 1u) {
            result *= base;
        }
        base *= base;
        power >>= 1;
    }
    return (float)result;
}
