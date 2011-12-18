#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#define MAX_PATH 4096

// Explicitly allow
char *allowed[] = {
    "/usr/include/_G_config.h",
    "/usr/include/alloca.h",
    "/usr/include/dlfcn.h",
    "/usr/include/stdio.h",
    "/usr/include/stdlib.h",
    "/usr/include/endian.h",
    "/usr/include/features.h",
    "/usr/include/fcntl.h",
    "/usr/include/getopt.h",
    "/usr/include/libio.h",
    "/usr/include/string.h",
    "/usr/include/time.h",
    "/usr/include/unistd.h",
    "/usr/include/xlocale.h",
    "/usr/include/wchar.h",
    "/usr/lib/gcc/",
    "/usr/include/x86_64-linux-gnu/",
    "/tmp",
};
#define Nallowed sizeof(allowed) / sizeof(char*)

// Forbid silently
char *ignore[] = {
    "/usr/local/include/",
};
#define Nignore sizeof(ignore) / sizeof(char*)

// Forbid and report
char *forbidden[] = {
    "/usr",
};
#define Nforbidden sizeof(forbidden) / sizeof(char*)

void expand_path(const char *pathname, char fullpath[MAX_PATH])
{
    if (pathname == NULL || strlen(pathname) >= MAX_PATH ||
            strlen(pathname) == 0) {
        printf("RESTRICT: internal error in expand_path()");
        abort();
    }
    if (pathname[0] == '/') {
        strcpy(fullpath, pathname);
        return;
    }
    getcwd(fullpath, MAX_PATH - 10);
    strncat(fullpath, "/", 1);
    strncat(fullpath, pathname, MAX_PATH-strlen(fullpath)-10);
}


/*
Tests if 'pathname' can be opened.

Returns 1 if it can, 0 if it cannot.
*/
int can_open(const char *pathname)
{
    static int logfd=-1;
    static int (*_open)(const char *, int, mode_t) = NULL;
    int i;
    char fullpath[MAX_PATH];
    if (logfd == -1) {
        _open = dlsym(RTLD_NEXT, "open");
        logfd = _open("/tmp/access.log", O_WRONLY|O_APPEND|O_CREAT, 0666);
    }
    expand_path(pathname, fullpath);
    for (i=0; i < Nallowed; i++) {
        if (strncmp(fullpath, allowed[i], strlen(allowed[i])) == 0)
            return 1;
    }
    for (i=0; i < Nignore; i++) {
        if (strncmp(fullpath, ignore[i], strlen(ignore[i])) == 0)
            return 0;
    }
    for (i=0; i < Nforbidden; i++) {
        if (strncmp(fullpath, forbidden[i], strlen(forbidden[i])) == 0) {
            if (logfd != -1) {
                dprintf(logfd, "RESTRICT: %s (due to '%s')\n", fullpath,
                        forbidden[i]);
            }
            return 0;
        }
    }
    if (logfd != -1) {
        dprintf(logfd, "Local file: %s\n", fullpath);
    }
    return 1;
}

int open(const char *pathname, int flags, ...)
{
    static int (*_open)(const char *, int, mode_t) = NULL;
    mode_t mode;
    va_list vl;
    va_start(vl, flags);
    mode = va_arg(vl, mode_t);
    va_end(vl);

    if (!_open) {
        _open = dlsym(RTLD_NEXT, "open");
    }
    if (can_open(pathname)) {
        return _open(pathname, flags, mode);
    } else {
        return -1;
    }
}

int open64(const char *pathname, int flags, ...)
{
    static int (*_open64)(const char *, int, mode_t) = NULL;
    mode_t mode;
    va_list vl;
    va_start(vl, flags);
    mode = va_arg(vl, mode_t);
    va_end(vl);

    if (!_open64) {
        _open64 = dlsym(RTLD_NEXT, "open64");
    }
    if (can_open(pathname)) {
        return _open64(pathname, flags, mode);
    } else {
        return -1;
    }
}

FILE *fopen(const char *pathname, const char *mode)
{
    static FILE *(*_fopen)(const char *, const char *) = NULL;

    if (!_fopen) {
        _fopen = dlsym(RTLD_NEXT, "fopen");
    }
    if (can_open(pathname)) {
        return _fopen(pathname, mode);
    } else {
        return NULL;
    }
}
