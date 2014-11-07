#ifdef SYSCALLS
extern int errno;

int _kill(int pid, int sig)
{
//    pid = pid; sig = sig; /* avoid warnings */
#define EINVAL          22
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
//    xprintf("_exit called with parameter %d\n", status);
     while(1) {;}
}

int _getpid(void)
{
    return 1;
}

int _close(int file)
{
//    file = file; /* avoid warning
    return -1;
}

int _fstat(int file, void *st)
{
//    file = file; /* avoid warning */
    return 0;
}

int _isatty(int file)
{
//    file = file; /* avoid warning */
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
//    file = file; /* avoid warning
//    ptr = ptr; /* avoid warning
//    dir = dir; /* avoid warning
    return 0;
}

int _read(int file, char *ptr, int len)
{
//    file = file; /* avoid warning */
//    ptr = ptr; /* avoid warning */
//    len = len; /* avoid warning */
    return 0;
}

int _write(int file, char *ptr, int len)
{
    int todo;
//    file = file; /* avoid warning */
    for (todo = 0; todo < len; todo++)
    {
//        xputc(*ptr++);
    }
    return len;
}

void fsync() {}

extern unsigned long _ebss;

void * _sbrk(int incr)
{
    static char *heap_end = (char *)&_ebss;

    void* sp;
    asm("mov %0, sp" : "=r"(sp) : : );
    if ((char*)sp <= heap_end+incr) {
        return (void*)-1;
    }

    char *prev = heap_end;

    heap_end += incr;
    return prev;
}


#endif
