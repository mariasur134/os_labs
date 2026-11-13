#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

#define NAME_SIZE 256
#define SHARED_OBJ_SIZE 4096

typedef struct {
    char shm_name[NAME_SIZE];
    char sem_full_name[NAME_SIZE];
    char sem_empty_name[NAME_SIZE];
    int shm_object;
    char* shm_buf;
    sem_t* sem_full;
    sem_t* sem_empty;
}channel;


int create_channel(channel *ch, int num, int parent_pid) {
    snprintf(ch->sem_full_name, NAME_SIZE, "/sem_full_%d_%d", parent_pid, num);

    snprintf(ch->sem_empty_name, NAME_SIZE, "/sem_empty_%d_%d", parent_pid, num);

    snprintf(ch->shm_name, NAME_SIZE, "/shm_%d_%d", parent_pid, num);


    ch->shm_object = shm_open(ch->shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (ch->shm_object == -1) {
        write(2, "shm_open error\n", 15);
        return -1;
    }

    if (ftruncate(ch->shm_object, SHARED_OBJ_SIZE) < 0) {
        close(ch->shm_object);
        shm_unlink(ch->shm_name);
        write(2, "ftruncate error\n", 16);
        return -1;
    }

    ch->shm_buf = mmap(NULL, SHARED_OBJ_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ch->shm_object, 0);
    if (ch->shm_buf == MAP_FAILED) {
        close(ch->shm_object);
        shm_unlink(ch->shm_name);
        write(2, "mmap error\n", 11);
        return -1;
    }


    ch->sem_full = sem_open(ch->sem_full_name, O_CREAT | O_EXCL, 0600, 0);
    if (ch->sem_full == SEM_FAILED) {
        munmap(ch->shm_buf, SHARED_OBJ_SIZE);
        close(ch->shm_object);
        shm_unlink(ch->shm_name);
        write(2, "sem_open error\n", 15);
        return -1;
    }

    ch->sem_empty = sem_open(ch->sem_empty_name, O_CREAT | O_EXCL, 0600, 1);
    if (ch->sem_empty == SEM_FAILED) {
        munmap(ch->shm_buf, SHARED_OBJ_SIZE);
        close(ch->shm_object);
        sem_unlink(ch->sem_full_name);
        sem_close(ch->sem_full);
        shm_unlink(ch->shm_name);
        write(2, "sem_open error\n", 15);
        return -1;
    }
    return 0;
}

void destroy_channel(channel *ch) {
    munmap(ch->shm_buf, SHARED_OBJ_SIZE);
    shm_unlink(ch->shm_name);
    close(ch->shm_object);
    sem_close(ch->sem_full);
    sem_close(ch->sem_empty);
    sem_unlink(ch->sem_full_name);
    sem_unlink(ch->sem_empty_name);
}


int main() {
    char file1[NAME_SIZE];
    const int n1 = read(0, file1, 256);
    char file2[NAME_SIZE];
    const int n2 = read(0, file2, 256);
    if (n1 <= 0 || n2 <= 0) {
        write(2, "file's name error\n", 18);
        return 1;
    }
    file1[n1 - 1] = '\0';
    file2[n2 - 1] = '\0';

    const int  file1_descr = open(file1, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    const int file2_descr = open(file2, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if ((file1_descr < 0) || (file2_descr < 0)) {
        write(2, "file error\n", 11);
        return 1;
    }


    // shared memory, creating channels
    int parent_pid = getpid();
    channel ch1, ch2;

    if (create_channel(&ch1, 1, parent_pid) < 0) {
        return 1;
    }
    if (create_channel(&ch2, 2, parent_pid) < 0) {
        destroy_channel(&ch1);
        return 1;
    }


    pid_t pid1 = fork();
    if (pid1 < 0) {
        write(2, "fork error\n", 11);
        return 1;
    }
    if (pid1 == 0) {
        int d2 = dup2(file1_descr, 1);
        if (d2 < 0){
            write(2, "dup2 error\n", 11);
            return 1;
        }

        close(file1_descr);
        close(file2_descr);

        execl("./child", "child", ch1.sem_full_name, ch1.sem_empty_name, ch1.shm_name, NULL);

        write(2, "child1 error\n", 13);
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        write(2, "fork error\n", 11);
        return 1;
    }
    if (pid2 == 0) {
        int d2 = dup2(file2_descr, 1);
        if (d2 < 0){
            write(2, "dup2 error\n", 11);
            return 1;
        }

        close(file1_descr);
        close(file2_descr);

        execl("./child", "child", ch2.sem_full_name, ch2.sem_empty_name, ch2.shm_name, NULL);

        write(2, "child2 error\n", 13);
        _exit(1);
    }

    close(file1_descr);
    close(file2_descr);

    char buf[4096];
    ssize_t bytes;
    int line_num = 0;

    while ((bytes = read(0, buf, 4096))){
        if (bytes < 0) {
            write(2, "input error\n", 12);
            return 1;
        }

        int start = 0;
        for (int i = 0; i < bytes; i++) {
            if (buf[i] == '\n') {
                channel *ch = (line_num % 2 == 0) ? &ch1 : &ch2;
                uint32_t length = i - start + 1;

                sem_wait(ch->sem_empty);
                *(uint32_t *)ch->shm_buf = length;
                for (uint32_t k = 0; k < length; k++) {
                    ch->shm_buf[sizeof(uint32_t) + k] = buf[start + k];
                }
                sem_post(ch->sem_full);

                line_num++;
                start = i + 1;
            }
        }
    }

    sem_wait(ch1.sem_empty);
    *(uint32_t *)ch1.shm_buf = UINT32_MAX;
    sem_post(ch1.sem_full);

    sem_wait(ch2.sem_empty);
    *(uint32_t *)ch2.shm_buf = UINT32_MAX;
    sem_post(ch2.sem_full);

    int status = 0;
    pid_t wait_pid1 = waitpid(pid1, &status, 0);
    pid_t wait_pid2 = waitpid(pid2, &status, 0);
    if (wait_pid1 < 0 || wait_pid2 < 0) {
        write(2, "waitpid error\n", 14);
        return 1;
    }

    destroy_channel(&ch1);
    destroy_channel(&ch2);

    return 0;
}
