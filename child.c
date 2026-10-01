#include <unistd.h>


int is_vowel(char c) {
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'
        || c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U'){
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    char buf[4098];
    ssize_t n;

    while ((n = read(0, buf, 4098)) > 0){
        ssize_t j = 0;
        for (ssize_t i = 0; i < n; i++) {
            if (!is_vowel(buf[i])) {
                buf[j] = buf[i];
                j++;
            }
        }
        if (write(1, buf, j) < 0) {
            write(2, "write error\n", 12);
            return 1;
        };
    }
    if (n < 0) {
        write(2, "read error\n", 11);
        return 1;
    }

    return 0;
}