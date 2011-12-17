#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdarg.h>


/*
Tests if 'pathname' can be opened.

Returns 1 if it can, 0 if it cannot.
*/
int can_open(const char *pathname)
{
    printf("file: %s\n", pathname);
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
