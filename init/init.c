#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;

    fd = open("/dev/tty0", O_RDWR);
    write(fd, "hello tty!\n", 11);

    return close(fd);
}
