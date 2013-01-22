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

/*
   Compile-time parameters
*/

#ifndef EXIT_CODE
#define EXIT_CODE 30
#endif

#ifndef EXIT_HEADER
#define EXIT_HEADER "hdistjail.so: "
#endif


/*
   Hook function declarations. Since this is a lot of repetetive code we rely on templating.

   Throughout, "p" is used as the path name to filter.
*/

/* rtype, funcname, decl_args, call_args */









static FILE * (*real_fopen)(const char *p, const char *mode) = NULL;

static int (*real_execve)(const char *p, char *const argv[], char *const envp[]) = NULL;

static int (*real_execv)(const char *p, char *const argv[]) = NULL;

static int (*real_access)(const char *p, int mode) = NULL;

static int (*real___xstat)(int x, const char *p, struct stat *buf) = NULL;

static int (*real___xstat64)(int x, const char *p, struct stat *buf) = NULL;

static int (*real_execvpe)(const char *p, char *const argv[], char *const envp[]) = NULL;

static int (*real_execvp)(const char *p, char *const argv[]) = NULL;

static int (*real_open)(const char *p, int oflag, mode_t mode) = NULL;

static int (*real_open64)(const char *p, int oflag, mode_t mode) = NULL;




/*
    string set
*/
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


/* like malloc() but calls exit() if malloc can't be performed */
static void *checked_malloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "%sOut of memory\n", EXIT_HEADER);
        exit(EXIT_CODE);
    }
    return p;
}



/*
    logging
*/
static int g_log_fd = -1;
static char *g_log_buf = NULL;
#define STDERR_PREFIX_SIZE 100
static char g_stderr_prefix[STDERR_PREFIX_SIZE];

static void open_log_file(void) {
    const char *log_filename = getenv("HDIST_JAIL_LOG");
    if (!log_filename) return;
    g_log_buf = checked_malloc(PIPE_BUF);
    g_log_fd = (*real_open)(log_filename, O_APPEND | O_CREAT | O_WRONLY, 0600);
    if (g_log_fd == -1) {
        fprintf(stderr, "Could not create log file (%s): %s",
                strerror(errno), log_filename);
        exit(EXIT_CODE);
    }
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
    if (g_log_fd != -1) {
        close(g_log_fd);
        free(g_log_buf);
    }
}

static void log_access(const char *path, const char *funcname) {
    ssize_t n, n_written;
    if (g_log_fd != -1) {
        n = snprintf(g_log_buf, PIPE_BUF,
                     "%d %s// %s\n", (int)getpid(), path, funcname);
        /* in the case of extremely long message (long filename?), do something
           half-way sane */
        g_log_buf[PIPE_BUF - 2] = '<';
        g_log_buf[PIPE_BUF - 1] = '\n';
        write(g_log_fd, g_log_buf, n);
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

static void load_real_funcs();

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


/*
    Hook implementation
*/


static void load_real_funcs() {
    
    real_fopen = dlsym(RTLD_NEXT, "fopen");
    
    real_execve = dlsym(RTLD_NEXT, "execve");
    
    real_execv = dlsym(RTLD_NEXT, "execv");
    
    real_access = dlsym(RTLD_NEXT, "access");
    
    real___xstat = dlsym(RTLD_NEXT, "__xstat");
    
    real___xstat64 = dlsym(RTLD_NEXT, "__xstat64");
    
    real_execvpe = dlsym(RTLD_NEXT, "execvpe");
    
    real_execvp = dlsym(RTLD_NEXT, "execvp");
    
    real_open = dlsym(RTLD_NEXT, "open");
    
    real_open64 = dlsym(RTLD_NEXT, "open64");
    
}


/* Simple functions that just forwards arguments */


FILE * fopen(const char *p, const char *mode) {
    if (!jail_access(p, "fopen")) return NULL;
    else return real_fopen(p, mode);
}


int execve(const char *p, char *const argv[], char *const envp[]) {
    if (!jail_access(p, "execve")) return -1;
    else return real_execve(p, argv, envp);
}


int execv(const char *p, char *const argv[]) {
    if (!jail_access(p, "execv")) return -1;
    else return real_execv(p, argv);
}


int access(const char *p, int mode) {
    if (!jail_access(p, "access")) return -1;
    else return real_access(p, mode);
}


int __xstat(int x, const char *p, struct stat *buf) {
    if (!jail_access(p, "stat")) return -1;
    else return real___xstat(x, p, buf);
}


int __xstat64(int x, const char *p, struct stat *buf) {
    if (!jail_access(p, "stat")) return -1;
    else return real___xstat64(x, p, buf);
}


/* For open()/open64() we need to treat varargs */

int open(const char *p, int oflag, ...) {
    if (oflag & O_CREAT) {
        va_list vl;
        mode_t mode;
        va_start(vl, oflag);
        mode = va_arg(vl, mode_t);
        va_end(vl);
        if (!jail_access(p, "open")) return -1;
        else return real_open(p, oflag, mode);
    } else {
        int (*two_arg_open)(const char *p, int oflag);
        two_arg_open = (void*)real_open;
        if (!jail_access(p, "open")) return -1;
        else return two_arg_open(p, oflag);
    }
}

int open64(const char *p, int oflag, ...) {
    if (oflag & O_CREAT) {
        va_list vl;
        mode_t mode;
        va_start(vl, oflag);
        mode = va_arg(vl, mode_t);
        va_end(vl);
        if (!jail_access(p, "open64")) return -1;
        else return real_open64(p, oflag, mode);
    } else {
        int (*two_arg_open)(const char *p, int oflag);
        two_arg_open = (void*)real_open64;
        if (!jail_access(p, "open64")) return -1;
        else return two_arg_open(p, oflag);
    }
}


/* For execvp*(), we need to check if p contains /. If not,
   we currently always pass it through (which is a bug, see README). */



