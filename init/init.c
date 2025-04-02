extern void print(char *);
extern int fork();
extern int execve(char *, char **, char**);
extern void _exit(int status);
extern int waitpid(int pid, int *wstatus, int options);
extern unsigned int alarm(unsigned int seconds);
extern int kill(int pid, int sig);
extern int signal(int signum, void *handler);

extern int errno;

void handle_timer()
{
    signal(14, (void*)handle_timer);
    alarm(1);
    print("got alarm signal!\n");
}

int main()
{
    signal(14, (void*)handle_timer);
    alarm(1);
    for (;;);
    return 0;
}
