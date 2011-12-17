#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>

// Explicitly allow
char *allowed[] = {
    "/usr/include/_G_config.h",
    "/usr/include/dlfcn.h",
    "/usr/include/stdio.h",
    "/usr/include/endian.h",
    "/usr/include/features.h",
    "/usr/include/libio.h",
    "/usr/include/string.h",
    "/usr/include/time.h",
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
    "/",
};
#define Nforbidden sizeof(forbidden) / sizeof(char*)



/*
Tests if 'pathname' can be opened.

Returns 1 if it can, 0 if it cannot.
*/
int can_open(const char *pathname)
{
    int i;
    for (i=0; i < Nallowed; i++) {
        if (strncmp(pathname, allowed[i], strlen(allowed[i])) == 0)
            return 1;
    }
    for (i=0; i < Nignore; i++) {
        if (strncmp(pathname, ignore[i], strlen(ignore[i])) == 0)
            return 0;
    }
    for (i=0; i < Nforbidden; i++) {
        if (strncmp(pathname, forbidden[i], strlen(forbidden[i])) == 0) {
            printf("RESTRICT: %s (due to '%s')\n", pathname, forbidden[i]);
            return 0;
        }
    }
    printf("Local file: %s\n", pathname);
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
