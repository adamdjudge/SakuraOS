#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;

    char *argv[] = {"hello", "world", 0};
    char *envp[] = {"foo=1", "bar=2", 0};

    if ((fd = open("/dev/tty0", O_RDWR)) < 0)
        return 1;
    dup(fd);
    dup(fd);

    if (fork() == 0) {
        execve("/bin/sh", argv, envp);
        write(STDERR_FILENO, "init: could not start shell\n", 28);
        return 2;
    } else {
        for (;;);
    }

    return 0;
}
