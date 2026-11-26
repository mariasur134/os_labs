#include <dlfcn.h>
#include <unistd.h>

#include "common.h"
#include "contracts.h"
#include <dlfcn.h>
#include <unistd.h>

#include "common.h"
#include "contracts.h"

#define LIB_COUNT 2

static void print_dl_error(const char *what, const char *path) {
    const char *msg = dlerror();
    out_str(STDERR_FILENO, "error: ");
    out_str(STDERR_FILENO, what);
    out_str(STDERR_FILENO, " ");
    out_str(STDERR_FILENO, path);
    if (msg != NULL) {
        out_str(STDERR_FILENO, ": ");
        out_str(STDERR_FILENO, msg);
    }
    out_str(STDERR_FILENO, "\n");
}

static void print_current(int index, const char *path) {
    out_str(STDOUT_FILENO, "implementation ");
    out_int(STDOUT_FILENO, index + 1);
    out_str(STDOUT_FILENO, " (");
    out_str(STDOUT_FILENO, path);
    out_str(STDOUT_FILENO, ")\n");
}

int main(int argc, char **argv) {
    const char *paths[LIB_COUNT] = {"./libimpl1.so", "./libimpl2.so"};
    void *handles[LIB_COUNT] = {NULL, NULL};
    prime_count_fn prime_count_impl[LIB_COUNT] = {NULL, NULL};
    e_fn e_impl[LIB_COUNT] = {NULL, NULL};
    int status = 0;

    if (argc == 3) {
        paths[0] = argv[1];
        paths[1] = argv[2];
    } else if (argc != 1) {
        out_str(STDERR_FILENO, "usage: prog2 [<lib1> <lib2>]\n");
        return 1;
    }

    for (int i = 0; i < LIB_COUNT; i++) {
        handles[i] = dlopen(paths[i], RTLD_LOCAL | RTLD_NOW);
        if (handles[i] == NULL) {
            print_dl_error("cannot load", paths[i]);
            status = 1;
            goto cleanup;
        }

        prime_count_impl[i] = (prime_count_fn)dlsym(handles[i], "prime_count");
        if (prime_count_impl[i] == NULL) {
            print_dl_error("cannot find prime_count in", paths[i]);
            status = 1;
            goto cleanup;
        }

        e_impl[i] = (e_fn)dlsym(handles[i], "e");
        if (e_impl[i] == NULL) {
            print_dl_error("cannot find e in", paths[i]);
            status = 1;
            goto cleanup;
        }
    }

    int current = 0;
    print_current(current, paths[current]);

    char line[1024];
    for (;;) {
        int truncated;
        ssize_t len = read_line(line, sizeof(line), &truncated);
        if (len == -1) {
            break;
        }
        if (len == -2) {
            out_str(STDERR_FILENO, "error: read failed\n");
            status = 1;
            break;
        }
        if (truncated) {
            out_str(STDERR_FILENO, "error: line is too long\n");
            continue;
        }

        const char *p = line;
        long long cmd;
        int r = next_int(&p, &cmd);
        if (r == PARSE_END) {
            continue;
        }
        if (r != PARSE_OK) {
            out_str(STDERR_FILENO, "error: invalid command\n");
            continue;
        }

        if (cmd == 0) {
            current = (current + 1) % LIB_COUNT;
            print_current(current, paths[current]);
        } else if (cmd == 1) {
            run_prime_count(prime_count_impl[current], p);
        } else if (cmd == 2) {
            run_e(e_impl[current], p);
        } else {
            out_str(STDERR_FILENO, "error: unknown command\n");
        }
    }

cleanup:
    for (int i = 0; i < LIB_COUNT; i++) {
        if (handles[i] != NULL) {
            dlclose(handles[i]);
        }
    }
    return status;
}
