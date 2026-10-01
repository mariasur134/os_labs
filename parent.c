#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


int main() {

    char file1[256];
    int n1 = read(0, file1, 256);
    char file2[256];
    int n2 = read(0, file2, 256);
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


    int pipe1[2];
    int pipe2[2];
    if ((pipe(pipe1) < 0) || (pipe(pipe2) < 0)) {
        write(2, "pipe error\n", 11);
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        write(2, "fork error\n", 11);
        return 1;
    }
    if (pid1 == 0) {
        int d1 = dup2(pipe1[0], 0);
        int d2 = dup2(file1_descr, 1);

        if (d1 < 0 || d2 < 0){
            write(2, "dup2 error\n", 11);
            return 1;
        }

        close(pipe1[0]);
        close(pipe1[1]);
        close(file1_descr);

        close(pipe2[0]);
        close(pipe2[1]);
        close(file2_descr);

        execl("./child", "child", NULL);

        write(2, "child1 error\n", 13);
        _exit(1);
    }


    pid_t pid2 = fork();
    if (pid2 < 0) {
        write(2, "fork error\n", 11);
        return 1;
    }
    if (pid2 == 0) {
        int d1 = dup2(pipe2[0], 0);
        int d2 = dup2(file2_descr, 1);

        if (d1 < 0 || d2 < 0){
            write(2, "dup2 error\n", 11);
            return 1;
        }

        close(pipe1[0]);
        close(pipe1[1]);
        close(file1_descr);

        close(pipe2[0]);
        close(pipe2[1]);
        close(file2_descr);

        execl("./child", "child", NULL);

        write(2, "child2 error\n", 13);
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[0]);
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
                int res_write = 0;

                if (line_num % 2 == 0) {
                    res_write = write(pipe1[1], buf + start, i - start + 1);
                }
                else res_write = write(pipe2[1], buf + start, i - start + 1);

                if (res_write < 0) {
                    write(2, "write error\n", 12);
                    return 1;
                }

                line_num++;
                start = i + 1;
            }
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    int status = 0;
    pid_t wait_pid1 = waitpid(pid1, &status, 0);
    pid_t wait_pid2 = waitpid(pid2, &status, 0);
    if (wait_pid1 < 0 || wait_pid2 < 0) {
        write(2, "waitpid error\n", 14);
        return 1;
    }
    return 0;
}
