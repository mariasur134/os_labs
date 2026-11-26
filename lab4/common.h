#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <stddef.h>
#include <unistd.h>

#include "contracts.h"

#define PARSE_OK     1
#define PARSE_END    0
#define PARSE_BAD   -1
#define PARSE_RANGE -2

static inline size_t str_len(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

static inline void out_str(int fd, const char *s) {
    size_t left = str_len(s);
    while (left > 0) {
        ssize_t w = write(fd, s, left);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        s += w;
        left -= (size_t)w;
    }
}

static inline void out_uint(int fd, unsigned long long v) {
    char buf[24];
    int pos = (int)sizeof(buf) - 1;
    buf[pos] = '\0';
    do {
        buf[--pos] = (char)('0' + v % 10);
        v /= 10;
    } while (v > 0);
    out_str(fd, buf + pos);
}

static inline void out_int(int fd, long long v) {
    if (v < 0) {
        out_str(fd, "-");
        out_uint(fd, (unsigned long long)(-(v + 1)) + 1ULL);
    } else {
        out_uint(fd, (unsigned long long)v);
    }
}


static inline void out_float(int fd, float value) {
    double v = value;
    if (v != v) {
        out_str(fd, "nan");
        return;
    }
    if (v < 0) {
        out_str(fd, "-");
        v = -v;
    }
    if (v > 1e18) {
        out_str(fd, "inf");
        return;
    }

    unsigned long long int_part = (unsigned long long)v;
    double frac = v - (double)int_part;
    unsigned long long scaled = (unsigned long long)(frac * 1000000.0 + 0.5);
    if (scaled >= 1000000ULL) {
        int_part++;
        scaled -= 1000000ULL;
    }

    out_uint(fd, int_part);
    out_str(fd, ".");

    char digits[7];
    for (int i = 5; i >= 0; i--) {
        digits[i] = (char)('0' + scaled % 10);
        scaled /= 10;
    }
    digits[6] = '\0';
    out_str(fd, digits);
}

static char in_buf[4096];
static size_t in_pos = 0;
static size_t in_len = 0;


static inline int in_getc(void) {
    if (in_pos == in_len) {
        ssize_t n;
        do {
            n = read(STDIN_FILENO, in_buf, sizeof(in_buf));
        } while (n < 0 && errno == EINTR);

        if (n < 0) {
            return -2;
        }
        if (n == 0) {
            return -1;
        }
        in_pos = 0;
        in_len = (size_t)n;
    }
    return (unsigned char)in_buf[in_pos++];
}

static inline ssize_t read_line(char *line, size_t cap, int *truncated) {
    size_t len = 0;
    int got_any = 0;
    *truncated = 0;

    for (;;) {
        int c = in_getc();
        if (c == -2) {
            return -2;
        }
        if (c == -1) {
            if (!got_any) {
                return -1;
            }
            break;
        }
        got_any = 1;
        if (c == '\n') {
            break;
        }
        if (c == '\r') {
            continue;
        }
        if (len + 1 < cap) {
            line[len++] = (char)c;
        } else {
            *truncated = 1;
        }
    }
    line[len] = '\0';
    return (ssize_t)len;
}

static inline int next_int(const char **p, long long *out) {
    const char *s = *p;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    if (*s == '\0') {
        *p = s;
        return PARSE_END;
    }

    int negative = 0;
    if (*s == '-' || *s == '+') {
        negative = (*s == '-');
        s++;
    }
    if (*s < '0' || *s > '9') {
        return PARSE_BAD;
    }

    long long value = 0;
    int range_error = 0;
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        if (value > 2147483648LL) {
            range_error = 1;
            value = 2147483648LL;
        }
        s++;
    }
    if (*s != '\0' && *s != ' ' && *s != '\t') {
        return PARSE_BAD;
    }

    if (negative) {
        value = -value;
    }
    if (range_error || value > 2147483647LL || value < -2147483648LL) {
        return PARSE_RANGE;
    }

    *p = s;
    *out = value;
    return PARSE_OK;
}

static inline int parse_args(const char *p, long long *args, int count) {
    for (int i = 0; i < count; i++) {
        int r = next_int(&p, &args[i]);
        if (r == PARSE_END) {
            out_str(STDERR_FILENO, "error: not enough arguments\n");
            return -1;
        }
        if (r == PARSE_BAD) {
            out_str(STDERR_FILENO, "error: argument is not an integer\n");
            return -1;
        }
        if (r == PARSE_RANGE) {
            out_str(STDERR_FILENO, "error: argument is out of int range\n");
            return -1;
        }
    }

    long long extra;
    if (next_int(&p, &extra) != PARSE_END) {
        out_str(STDERR_FILENO, "error: too many arguments\n");
        return -1;
    }
    return 0;
}



static inline void run_prime_count(prime_count_fn fn, const char *args_str) {
    long long args[2];
    if (parse_args(args_str, args, 2) != 0) {
        return;
    }
    if (args[0] < 1 || args[1] < 1) {
        out_str(STDERR_FILENO, "error: a and b must be natural numbers\n");
        return;
    }
    if (args[0] > args[1]) {
        out_str(STDERR_FILENO, "error: a must not be greater than b\n");
        return;
    }

    int result = fn((int)args[0], (int)args[1]);
    if (result < 0) {
        out_str(STDERR_FILENO, "error: not enough memory\n");
        return;
    }

    out_str(STDOUT_FILENO, "prime_count(");
    out_int(STDOUT_FILENO, args[0]);
    out_str(STDOUT_FILENO, ", ");
    out_int(STDOUT_FILENO, args[1]);
    out_str(STDOUT_FILENO, ") = ");
    out_int(STDOUT_FILENO, result);
    out_str(STDOUT_FILENO, "\n");
}

static inline void run_e(e_fn fn, const char *args_str) {
    long long args[1];
    if (parse_args(args_str, args, 1) != 0) {
        return;
    }
    if (args[0] < 1) {
        out_str(STDERR_FILENO, "error: x must be a natural number\n");
        return;
    }

    float result = fn((int)args[0]);

    out_str(STDOUT_FILENO, "e(");
    out_int(STDOUT_FILENO, args[0]);
    out_str(STDOUT_FILENO, ") = ");
    out_float(STDOUT_FILENO, result);
    out_str(STDOUT_FILENO, "\n");
}

#endif
