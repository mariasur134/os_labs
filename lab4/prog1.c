#include <unistd.h>

#include "common.h"
#include "contracts.h"

int main(void) {
    char line[1024];
    int status = 0;

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
            out_str(STDERR_FILENO, "switching is not supported in this program\n");
        } else if (cmd == 1) {
            run_prime_count(prime_count, p);
        } else if (cmd == 2) {
            run_e(e, p);
        } else {
            out_str(STDERR_FILENO, "error: unknown command\n");
        }
    }

    return status;
}