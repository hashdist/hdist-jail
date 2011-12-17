#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

int open(const char *pathname, int flags, mode_t mode)
{
    printf("RESTRICTED OPEN: %s\n", pathname);
    // You can abort using:
    //abort();
    return syscall(SYS_open, pathname, flags, mode);
}
