#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd, count;
    char buf[16];

    fd = open("/dev/tty0", O_RDWR);
    for (;;) {
        count = read(fd, buf, 16);
        if (count == 1 && buf[0] == '\n')
            break;
        write(fd, "read: ", 6);
        write(fd, buf, count);
        if (buf[count-1] != '\n')
            write(fd, "\n", 1);
    }

    return close(fd);
}
