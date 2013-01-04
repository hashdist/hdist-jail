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
    return (i != kh_end(h) && kh_exist(h, i));
}

/* iterates a khash_t(strset) and frees all keys, then destroys the set */
void strset_destroy(khash_t(strset) *h) {
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

#ifndef EXIT_CODE
#define EXIT_CODE 30
#endif

#ifndef EXIT_HEADER
#define EXIT_HEADER "hdistjail.so: "
#endif


/* like malloc() but calls exit() if malloc can't be performed */
static void *checked_malloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "%sOut of memory\n", EXIT_HEADER);
        exit(EXIT_CODE);
    }
    return p;
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
    logging
*/
static FILE *g_logfile = NULL;
#define STDERR_PREFIX_SIZE 100
static char g_stderr_prefix[STDERR_PREFIX_SIZE];

static void open_log_file(void) {
    char *filename;
    size_t prefix_len;
    int fd;
    const char *log_filename_prefix = getenv("HDIST_JAIL_LOG");
    if (!log_filename_prefix) return;
    prefix_len = strlen(log_filename_prefix);
    filename = checked_malloc(prefix_len + 1 + 10 + 1 + 6);
    strcpy(filename, log_filename_prefix);
    strcat(filename, "-");
    snprintf(filename + prefix_len + 1, 10, "%d", getpid());
    filename[prefix_len + 1 + 9] = 0; /* in case of overflow */
    strcat(filename, "-XXXXXX");
    fd = mkstemp(filename);
    free(filename);
    if (fd == -1) goto ERROR;
    g_logfile = fdopen(fd, "w");
    if (g_logfile == NULL) goto ERROR;
    return;
 ERROR:
    fprintf(stderr, "Could not create log file: %s", strerror(errno));
    exit(EXIT_CODE);
}

static void open_log(void) {
    open_log_file();
    {
        const char *stderr_prefix = getenv("HDIST_JAIL_STDERR");
        g_stderr_prefix[0] = 0;
        if (stderr_prefix) {
            strncpy(g_stderr_prefix, stderr_prefix, STDERR_PREFIX_SIZE - 1);
            g_stderr_prefix[STDERR_PREFIX_SIZE - 1] = 0;
        }
    }
}

static void close_log(void) {
    if (g_logfile) {
        fclose(g_logfile);
    }
}

static void log_access(const char *path, const char *funcname) {
    if (g_logfile) {
        fprintf(g_logfile, "%s// %s\n", path, funcname);
    }
    if (g_stderr_prefix[0]) {
        fprintf(stderr, "%s%s(\"%s\", ...)\n", g_stderr_prefix, funcname, path);
    }
}

/*
    whitelists
*/
static khash_t(strset) *g_whitelist_exact;
static khash_t(strset) *g_whitelist_prefixes;

static void create_whitelist(void) {
    g_whitelist_exact = kh_init(strset);
    g_whitelist_prefixes = kh_init(strset);
}

static void destroy_whitelist(void) {
    strset_destroy(g_whitelist_exact);
    strset_destroy(g_whitelist_prefixes);
}

/* p should be absolute/canonical path */
static int is_whitelisted(char *p) {
    int success = 0;
    if (strset_contains(g_whitelist_exact, p)) {
        /* exact match */
        success = 1;
    } else {
        /* Try all prefixes; q starts at end of p and then rewinds */
        size_t n = strlen(p);
        p = strdup(p);
        char *q = p + n;
        if (n > 0) {
            while (1) {
                /* rewind to previous patsep, set it to 0, and look up the prefix */
                while (q != p && *q != '/') --q;
                if (q == p) break;
                *q = 0;
                if (strset_contains(g_whitelist_prefixes, p)) {
                    success = 1;
                    break;
                }
            }
        }
        free(p);
    }
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
            kh_put(strset, g_whitelist_prefixes, line, &kh_ret);
            if (kh_ret == 0) {
                printf("already present #1\n");
                free(line);
            }
        } else {
            kh_put(strset, g_whitelist_exact, line, &kh_ret);
            if (kh_ret == 0) {
                printf("already present #2\n");
                free(line);
            }
        }
    }
}


/*
    jail handling
*/

static int g_should_hide;

static int jail_access(const char *arg_path, const char *funcname) {
    char *p = abspath(arg_path);
    int ret = 1;
    if (!is_whitelisted(p)) {
        log_access(p, funcname);
        if (g_should_hide) {
            errno = ENOENT;
            ret = 0;
        }
    }
    free(p);
    return ret;
}



/*
    initialization/finalization code
*/

__attribute__((constructor)) static void _init(void) {
    load_real_funcs();
    create_whitelist();
    open_log();
    {
        char *whitelist = getenv("HDIST_JAIL_WHITELIST");
        if (whitelist && strcmp(whitelist, "") != 0) {
            load_whitelist(whitelist);
        }
    }
    {
        char *mode = getenv("HDIST_JAIL_MODE");
        g_should_hide = 0;
        if (mode) {
            if ((strcmp(mode, "") == 0) || (strcmp(mode, "off") == 0)) {
            } else if (strcmp(mode, "hide") == 0) {
                g_should_hide = 1;
            } else {
                fprintf(stderr, "%sinvalid HDIST_JAIL_MODE: %s",
                        EXIT_HEADER, mode);
                exit(EXIT_CODE);
            }
        }
    }
}

__attribute__((destructor)) static void _finalize(void) {
    destroy_whitelist();
    close_log();
}

int can_open(const char *x) {return 1;}

int open(const char *pathname, int flags, ...)
{
    mode_t mode;
    va_list vl;
    va_start(vl, flags);
    mode = va_arg(vl, mode_t);
    va_end(vl);
    if (!jail_access(pathname, "open")) return -1;
    else return real_open(pathname, flags, mode);
}

int open64(const char *pathname, int flags, ...)
{
    mode_t mode;
    va_list vl;
    va_start(vl, flags);
    mode = va_arg(vl, mode_t);
    va_end(vl);
    if (!jail_access(pathname, "open64")) return -1;
    else return real_open64(pathname, flags, mode);
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
