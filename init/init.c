#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void handle_timer_parent()
{
    signal(SIGALRM, handle_timer_parent);
    alarm(1);
    print("hello from parent!\n");
}

void handle_timer_child()
{
    signal(SIGALRM, handle_timer_child);
    alarm(3);
    print("hello from child!\n");
}

int main()
{
    int ret = fork();
    if (ret < 0) {
        print("fork failed\n");
        _exit(-1);
    } else if (ret == 0) {
        signal(SIGALRM, handle_timer_child);
        alarm(3);
    } else {
        signal(SIGALRM, handle_timer_parent);
        alarm(1);
    }
    for (;;);
    return 0;
}
