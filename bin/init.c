#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd, count;
    char buf[16];

    fd = open("/dev/tty0", O_RDWR);
    dup(fd);
    dup(fd);

    for (;;) {
        count = read(STDIN_FILENO, buf, 16);
        if (count == 1 && buf[0] == '\n')
            break;
        write(STDOUT_FILENO, "read: ", 6);
        write(STDOUT_FILENO, buf, count);
        if (buf[count-1] != '\n')
            write(STDOUT_FILENO, "\n", 1);
    }

    return close(fd);
}
