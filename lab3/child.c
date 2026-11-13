#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdint.h>

#define SHARED_OBJ_SIZE 4096

int is_vowel(char c) {
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'
        || c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U'){
        return 1;
        }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        write(2, "Wrong number of arguments\n", 26);
        return 1;
    }

    // argv[1] - semaphor_full, argv[2] - semaphor_empty, argv[3] - shared object (string in my case)

    // getting pointer to an existing semaphore
    sem_t* sem_full = sem_open(argv[1], 0);
    if (sem_full == SEM_FAILED) {
        write(2, "sem_open error\n", 15);
        return 1;
    }

    // getting pointer to an existing semaphore
    sem_t* sem_empty = sem_open(argv[2], 0);
    if (sem_empty == SEM_FAILED) {
        sem_close(sem_full); // the 1st one opened, should close
        write(2, "sem_open error\n", 15);
        return 1;
    }

    // getting descriptor of an existing shared object
    // it's different from the descriptor that parent has
    int shm_obj = shm_open(argv[3], O_RDWR, 0);
    if (shm_obj < 0) {
        sem_close(sem_full);
        sem_close(sem_empty);
        write(2, "shm_open error\n", 15);
        return 1;
    }

    // getting a pointer to the object (we can now write and read)
    char *shm_buf = mmap(NULL, SHARED_OBJ_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_obj, 0);
    if (shm_buf == MAP_FAILED) {
        sem_close(sem_full);
        sem_close(sem_empty);
        close(shm_obj);
        write(2, "mmap error\n", 11);
        return 1;
    }

    char* string_buf = (char *) (shm_buf + sizeof(uint32_t));
    while (1) {
        ssize_t j = 0;

        sem_wait(sem_full);
        uint32_t length = *(uint32_t *)shm_buf;


        if (length == UINT32_MAX) { // input ended
            break;
        }

        if ((length > SHARED_OBJ_SIZE - sizeof(uint32_t)) || length < 1) {
            sem_close(sem_full);
            sem_close(sem_empty);
            close(shm_obj);
            munmap(shm_buf, SHARED_OBJ_SIZE);
            write(2, "len error\n", 10);
            return 1;
        }

        for (ssize_t i = 0; i < length; i++) {
            if (!is_vowel(string_buf[i])) {
                string_buf[j] = string_buf[i];
                j++;
            }
        }
        if (write(1, string_buf, j) < 0) {
            sem_close(sem_full);
            sem_close(sem_empty);
            close(shm_obj);
            munmap(shm_buf, SHARED_OBJ_SIZE);
            write(2, "write error\n", 12);
            return 1;
        };

        sem_post(sem_empty);
    }

    sem_close(sem_full);
    sem_close(sem_empty);
    close(shm_obj);
    munmap(shm_buf, SHARED_OBJ_SIZE);

    return 0;
}