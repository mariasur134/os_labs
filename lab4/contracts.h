#ifndef CONTRACTS_H
#define CONTRACTS_H

int prime_count(int a, int b);

float e(int x);

typedef int (*prime_count_fn)(int a, int b);
typedef float (*e_fn)(int x);

#endif
