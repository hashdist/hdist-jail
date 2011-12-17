#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

int open(const char *pathname, int flags, mode_t mode)
{
    printf("RESTRICT: %s: %s\n", __PRETTY_FUNCTION__, pathname);
    return syscall(SYS_open, pathname, flags, mode);
}
