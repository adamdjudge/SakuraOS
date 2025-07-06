#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(char **argv, char **envp)
{
    int ret;
    char buffer[64];
    char *s;

    write(STDOUT_FILENO, "argv:", 5);
    while (*argv) {
        write(STDOUT_FILENO, " ", 1);
        s = *argv++;
        while (*s)
            write(STDOUT_FILENO, s++, 1);
    }
    write(STDOUT_FILENO, "\nenvp:", 6);
    while (*envp) {
        write(STDOUT_FILENO, " ", 1);
        s = *envp++;
        while (*s)
            write(STDOUT_FILENO, s++, 1);
    }
    write(STDOUT_FILENO, "\n", 1);

    for (;;) {
        write(STDOUT_FILENO, "# ", 2);
        ret = read(STDIN_FILENO, buffer, sizeof(buffer)-1);
        if (ret < 0) {
            write(STDERR_FILENO, "sh: read error\n", 15);
            return 1;
        } else if (ret == 0)
            buffer[0] = '\0';
        else
            buffer[ret-1] = '\0';

        ret = fork();
        if (ret < 0)
            write(STDERR_FILENO, "sh: fork failed\n", 16);
        else if (ret > 0)
            wait(NULL);
        else {
            execve(buffer, NULL, NULL);
            write(STDERR_FILENO, "sh: bad command\n", 16);
            return 1;
        }
    }

    return 0;
}
