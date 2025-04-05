extern void print(char *);
extern int fork();
extern int execve(char *, char **, char**);
extern void _exit(int status);
extern int waitpid(int pid, int *wstatus, int options);
extern unsigned int alarm(unsigned int seconds);
extern int kill(int pid, int sig);
extern int signal(int signum, void *handler);

#define SIGALRM 14

extern int errno;

void handle_timer_parent()
{
    signal(SIGALRM, (void*)handle_timer_parent);
    alarm(1);
    print("hello from parent!\n");
}

void handle_timer_child()
{
    signal(SIGALRM, (void*)handle_timer_child);
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
        signal(SIGALRM, (void *)handle_timer_child);
        alarm(3);
    } else {
        signal(SIGALRM, (void *)handle_timer_parent);
        alarm(1);
    }
    for (;;);
    return 0;
}
