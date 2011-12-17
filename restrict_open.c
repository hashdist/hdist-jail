#include "res_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

void initres(void); /*proto*/

int python_initialized=0;

int open(const char *pathname, int flags, mode_t mode)
{
    if (!python_initialized) {
        Py_Initialize();
        initres();
        if (import_res()) {
            printf("Python failed to initialize, aborting...\n");
            abort();
        }
        python_initialized = 1;
    }
    if (can_open(pathname)) {
        return syscall(SYS_open, pathname, flags, mode);
    } else {
        printf("Not allowed to open file: %s\n", pathname);
        abort();
    }
}
