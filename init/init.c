#include <sys/types.h>
#include <unistd.h>

int main()
{
    int fd;
    ssize_t len;
    char buf[17];

    fd = open("test.txt", O_RDONLY);
    if (fd < 0) {
        print("open failed\n");
        return -1;
    }

    do {
        len = read(fd, buf, 16);
        buf[len] = 0;
        print(buf);
    } while (len > 0);

    return close(fd);
}
