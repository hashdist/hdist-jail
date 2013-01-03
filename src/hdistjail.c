#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "abspath.h"
#include "khash.h"

KHASH_SET_INIT_STR(strset)


static int strset_contains(khash_t(strset) *h, const char *key) {
   khint_t i = kh_get(strset, h, key);
   return kh_exist(h, i);
}

#ifndef EXIT_CODE
#define EXIT_CODE 30
#endif

#ifndef EXIT_HEADER
#define EXIT_HEADER "hdistjail.so: "
#endif


/* like malloc() but calls exit() if malloc can't be performed */
/*static void *checked_malloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "%sOut of memory\n", EXIT_HEADER);
        exit(EXIT_CODE);
    }
    return p;
    }*/

/* iterates a khash_t(strset) and frees all keys, then destroys the set */
void destroy_strset(khash_t(strset) *h) {
    khint_t it;
    for (it = kh_begin(h); it != kh_end(h); ++it) {
        if (kh_exist(h, it)) {
            const char *key = kh_key(h, it);
            kh_del(strset, h, it);
            free((void*)key);
        }
    }
    kh_destroy(strset, h);
}



static int (*real_open)(const char *, int, mode_t) = NULL;
static int (*real_open64)(const char *, int, mode_t) = NULL;
static FILE *(*real_fopen)(const char *, const char *) = NULL;
static int (*real_execve)(const char *, char *const argv[], char *const envp[]);
static int (*real_execvp)(const char *, char *const argv[]);
static int (*real_execv)(const char *, char *const argv[]);
static int (*real___xstat)(int x, const char *, struct stat *);
static int (*real___xstat64)(int x, const char *, struct stat64 *);
static int (*real_access)(const char *pathname, int mode);


static void load_real_funcs() {
    real_open = dlsym(RTLD_NEXT, "open");
    real_open64 = dlsym(RTLD_NEXT, "open64");
    real_fopen = dlsym(RTLD_NEXT, "fopen");
    real_access = dlsym(RTLD_NEXT, "access");
    real_execve = dlsym(RTLD_NEXT, "execve");
    real_execvp = dlsym(RTLD_NEXT, "execvp");
    real_execv = dlsym(RTLD_NEXT, "execv");
    real___xstat = dlsym(RTLD_NEXT, "__xstat");
    real___xstat64 = dlsym(RTLD_NEXT, "__xstat64");
}

/*
    whitelists
*/
static khash_t(strset) *whitelist_exact;
static khash_t(strset) *whitelist_prefixes;

static void create_whitelist(void) {
    whitelist_exact = kh_init(strset);
    whitelist_prefixes = kh_init(strset);
}

static void destroy_whitelist(void) {
    destroy_strset(whitelist_exact);
    destroy_strset(whitelist_prefixes);
}

static int is_whitelisted(const char *non_absolute_path) {
    char *p = abspath(non_absolute_path);
    int success = 0;
    if (strset_contains(whitelist_exact, p)) {
        /* exact match */
        success = 1;
    } else {
        /* Try all prefixes; q starts at end of p and then rewinds */
        size_t n = strlen(p);
        char *q = p + n;
        if (n > 0) {
            while (1) {
                /* rewind to previous patsep, set it to 0, and look up the prefix */
                while (q != p && *q != '/') --q;
                if (q == p) break;
                *q = 0;
                if (strset_contains(whitelist_prefixes, p)) {
                    success = 1;
                    break;
                }
            }
        }
    }
    free(p);
    return success;
}

static void load_whitelist(char *filename) {
    FILE *fd = real_fopen(filename, "r");
    if (fd == NULL) {
        fprintf(stderr, "%sError reading %s: %s\n",
                EXIT_HEADER, filename, strerror(errno));
        exit(EXIT_CODE);
    }
    while (1) {
        char *line = NULL;
        size_t n, r;
        int kh_ret;
        if ((r = getline(&line, &n, fd)) == -1) {
            break;
        }
        if (line[0] != '/') {
            fprintf(stderr, "%sAll entries in %s must be absolute paths\n",
                    EXIT_HEADER, filename);
            exit(EXIT_CODE);
        }        /* strip trailing newline if any */
        if (line[r - 1] == '\n') {
            line[r - 1] = 0;
            r--;
        }
        /* strip blank lines */
        if (strcmp(line, "") == 0) {
            continue;
        }
        /* check for "/<star><star>" suffix */
        if (r >= 3 && strcmp(&line[r - 3], "/**") == 0) {
            line[r - 3] = 0; /* truncate  */
            kh_put(strset, whitelist_prefixes, line, &kh_ret);
            if (kh_ret == 0) {
                printf("already present #1\n");
                free(line);
            }
        } else {
            kh_put(strset, whitelist_exact, line, &kh_ret);
            if (kh_ret == 0) {
                printf("already present #2\n");
                free(line);
            }
        }
    }
}


/*
    initialization/finalization code
*/

__attribute__((constructor)) static void _init(void) {
    load_real_funcs();
    create_whitelist();
    {
        char *whitelist = getenv("HDIST_JAIL_WHITELIST");
        if (whitelist != NULL && strcmp(whitelist, "") != 0) {
            load_whitelist(whitelist);
        }
    }
}

__attribute__((destructor)) static void _finalize(void) {
    destroy_whitelist();
}









/*
Tests if 'pathname' can be opened.

Returns 1 if it can, 0 if it cannot.
*/
int can_open(const char *pathname)
{
    return is_whitelisted(pathname);
}

int open(const char *pathname, int flags, ...)
{
    mode_t mode;
    va_list vl;
    va_start(vl, flags);
    mode = va_arg(vl, mode_t);
    va_end(vl);
    if (!can_open(pathname)) {
        errno = ENOENT;
        return -1;
    }
    return real_open(pathname, flags, mode);
}

int open64(const char *pathname, int flags, ...)
{
    mode_t mode;
    va_list vl;
    va_start(vl, flags);
    mode = va_arg(vl, mode_t);
    va_end(vl);
    if (!can_open(pathname)) {
        errno = ENOENT;
        return -1;
    }
    return real_open64(pathname, flags, mode);
}

FILE *fopen(const char *pathname, const char *mode)
{
    if (!can_open(pathname)) {
        errno = ENOENT;
        return NULL;
    }
    return real_fopen(pathname, mode);
}

int execve(const char *filename, char *const argv[], char *const envp[])
{
    static int (*_execve)(const char *, char *const argv[], char *const envp[]);

    if (!_execve) {
        _execve = dlsym(RTLD_NEXT, "execve");
    }
    if (can_open(filename)) {
        return _execve(filename, argv, envp);
    } else {
        return -1;
    }
}

int execvp(const char *filename, char *const argv[])
{
    static int (*_execvp)(const char *, char *const argv[]);

    if (!_execvp) {
        _execvp = dlsym(RTLD_NEXT, "execvp");
    }
    if (can_open(filename)) {
        return _execvp(filename, argv);
    } else {
        return -1;
    }
}

int execv(const char *filename, char *const argv[])
{
    static int (*_execv)(const char *, char *const argv[]);

    if (!_execv) {
        _execv = dlsym(RTLD_NEXT, "execv");
    }
    if (can_open(filename)) {
        return _execv(filename, argv);
    } else {
        return -1;
    }
}

// These are stat() and stat64() functions, that internally in GNU libc() are
// implemented as __xstat and __xstat64. See:
// http://stackoverflow.com/questions/5478780/c-and-ld-preload-open-and-open64-calls-intercepted-but-not-stat64

int __xstat(int x, const char *filename, struct stat *st)
{
    static int (*__xstat2)(int x, const char *, struct stat *);

    if (!__xstat2) {
        __xstat2 = dlsym(RTLD_NEXT, "__xstat");
    }
    if (can_open(filename)) {
        return __xstat2(x, filename, st);
    } else {
        return -1;
    }
}

int __xstat64(int x, const char *filename, struct stat64 *st)
{
    static int (*__xstat642)(int x, const char *, struct stat64 *);

    if (!__xstat642) {
        __xstat642 = dlsym(RTLD_NEXT, "__xstat64");
    }

    if (can_open(filename)) {
        return __xstat642(x, filename, st);
    } else {
        return -1;
    }
}

int access(const char *pathname, int mode)
{
    static int (*_access)(const char *pathname, int mode);

    if (!_access) {
        _access = dlsym(RTLD_NEXT, "access");
    }

    if (can_open(pathname)) {
        return _access(pathname, mode);
    } else {
        return -1;
    }
}
