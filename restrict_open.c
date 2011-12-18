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

// Explicitly allow
char *allowed[] = {
// libc6 includes:
    "/usr/include/netrose/",
    "/usr/include/gshadow.h",
    "/usr/include/regexp.h",
    "/usr/include/memory.h",
    "/usr/include/sysexits.h",
    "/usr/include/grp.h",
    "/usr/include/printf.h",
    "/usr/include/ucontext.h",
    "/usr/include/glob.h",
    "/usr/include/cpio.h",
    "/usr/include/link.h",
    "/usr/include/syscall.h",
    "/usr/include/ifaddrs.h",
    "/usr/include/ustat.h",
    "/usr/include/ttyent.h",
    "/usr/include/fcntl.h",
    "/usr/include/wchar.h",
    "/usr/include/ulimit.h",
    "/usr/include/error.h",
    "/usr/include/search.h",
    "/usr/include/ctype.h",
    "/usr/include/wait.h",
    "/usr/include/getopt.h",
    "/usr/include/regex.h",
    "/usr/include/netipx",
    "/usr/include/dlfcn.h",
    "/usr/include/spawn.h",
    "/usr/include/endian.h",
    "/usr/include/x86_64-linux-gnu/",
    "/usr/include/stropts.h",
    "/usr/include/ar.h",
    "/usr/include/protocols/",
    "/usr/include/unistd.h",
    "/usr/include/utime.h",
    "/usr/include/byteswap.h",
    "/usr/include/a.out.h",
    "/usr/include/netash",
    "/usr/include/netash/ash.h",
    "/usr/include/values.h",
    "/usr/include/re_comp.h",
    "/usr/include/crypt.h",
    "/usr/include/fmtmsg.h",
    "/usr/include/malloc.h",
    "/usr/include/nss.h",
    "/usr/include/shadow.h",
    "/usr/include/rpcsvc/",
    "/usr/include/netatalk/",
    "/usr/include/dirent.h",
    "/usr/include/execinfo.h",
    "/usr/include/_G_config.h",
    "/usr/include/nl_types.h",
    "/usr/include/tgmath.h",
    "/usr/include/utmpx.h",
    "/usr/include/aliases.h",
    "/usr/include/pthread.h",
    "/usr/include/neteconet/",
    "/usr/include/strings.h",
    "/usr/include/stdio_ext.h",
    "/usr/include/libintl.h",
    "/usr/include/sched.h",
    "/usr/include/utmp.h",
    "/usr/include/gnu-versions.h",
    "/usr/include/stab.h",
    "/usr/include/elf.h",
    "/usr/include/err.h",
    "/usr/include/syslog.h",
    "/usr/include/libio.h",
    "/usr/include/langinfo.h",
    "/usr/include/argp.h",
    "/usr/include/gconv.h",
    "/usr/include/netrom/",
    "/usr/include/netdb.h",
    "/usr/include/setjmp.h",
    "/usr/include/lastlog.h",
    "/usr/include/xlocale.h",
    "/usr/include/arpa/",
    "/usr/include/nfs",
    "/usr/include/nfs/nfs.h",
    "/usr/include/netpacket/",
    "/usr/include/netax25/",
    "/usr/include/ftw.h",
    "/usr/include/wordexp.h",
    "/usr/include/paths.h",
    "/usr/include/fstab.h",
    "/usr/include/mqueue.h",
    "/usr/include/thread_db.h",
    "/usr/include/termios.h",
    "/usr/include/stdio.h",
    "/usr/include/iconv.h",
    "/usr/include/ieee754.h",
    "/usr/include/netinet/",
    "/usr/include/envz.h",
    "/usr/include/monetary.h",
    "/usr/include/mntent.h",
    "/usr/include/assert.h",
    "/usr/include/errno.h",
    "/usr/include/limits.h",
    "/usr/include/resolv.h",
    "/usr/include/features.h",
    "/usr/include/fnmatch.h",
    "/usr/include/pwd.h",
    "/usr/include/mcheck.h",
    "/usr/include/wctype.h",
    "/usr/include/rpc/",
    "/usr/include/alloca.h",
    "/usr/include/complex.h",
    "/usr/include/stdint.h",
    "/usr/include/stdlib.h",
    "/usr/include/string.h",
    "/usr/include/semaphore.h",
    "/usr/include/signal.h",
    "/usr/include/poll.h",
    "/usr/include/fts.h",
    "/usr/include/scsi/",
    "/usr/include/inttypes.h",
    "/usr/include/sgtty.h",
    "/usr/include/pty.h",
    "/usr/include/obstack.h",
    "/usr/include/aio.h",
    "/usr/include/locale.h",
    "/usr/include/time.h",
    "/usr/include/netiucv/",
    "/usr/include/net/",
    "/usr/include/termio.h",
    "/usr/include/tar.h",
    "/usr/include/fenv.h",
    "/usr/include/math.h",
    "/usr/include/libgen.h",
    "/usr/include/argz.h",

// linux-libc
    "/usr/include/drm/",
    "/usr/include/sound/",
    "/usr/include/rdma/",
    "/usr/include/x86_64-linux-gnu/",
    "/usr/include/video/",
    "/usr/include/asm-generic/",
    "/usr/include/mtd/",
    "/usr/include/linux/",
    "/usr/include/xen/",

// gcc:
    "/usr/bin/gcc",
    "/usr/bin/ld",

// others:
    "/usr/include/c++/",
    "/usr/lib/gcc/",
    "/usr/lib/x86_64-linux-gnu",
    "/usr/lib/i386-linux-gnu",
    "/tmp",
    "/dev/null",
    "/dev/tty",
    "/dev/urandom",
    "/lib/charset.alias",
    "/usr/bin/install",
    "/etc/termcap",
    "/etc/ld.so.conf",
    "/etc/inputrc",
    "/proc",

    "/bin/sed",
    "/bin/tar",
    "/bin/grep",
    "/bin/dash",
    "/bin/hostname",
    "/bin/bash",

// coreutils:
    "/bin/cat",
    "/bin/chgrp",
    "/bin/chmod",
    "/bin/chown",
    "/bin/cp",
    "/bin/date",
    "/bin/dd",
    "/bin/df",
    "/bin/dir",
    "/bin/echo",
    "/bin/false",
    "/bin/ln",
    "/bin/ls",
    "/bin/mkdir",
    "/bin/mknod",
    "/bin/mv",
    "/bin/pwd",
    "/bin/readlink",
    "/bin/rm",
    "/bin/rmdir",
    "/bin/vdir",
    "/bin/sleep",
    "/bin/stty",
    "/bin/sync",
    "/bin/touch",
    "/bin/true",
    "/bin/uname",
    "/bin/mktemp",
    "/usr/bin",
    "/usr/bin/install",
    "/usr/bin/hostid",
    "/usr/bin/nice",
    "/usr/bin/who",
    "/usr/bin/users",
    "/usr/bin/pinky",
    "/usr/bin/stdbuf",
    "/usr/bin/[",
    "/usr/bin/base64",
    "/usr/bin/basename",
    "/usr/bin/chcon",
    "/usr/bin/cksum",
    "/usr/bin/comm",
    "/usr/bin/csplit",
    "/usr/bin/cut",
    "/usr/bin/dircolors",
    "/usr/bin/dirname",
    "/usr/bin/du",
    "/usr/bin/env",
    "/usr/bin/expand",
    "/usr/bin/expr",
    "/usr/bin/factor",
    "/usr/bin/fmt",
    "/usr/bin/fold",
    "/usr/bin/groups",
    "/usr/bin/head",
    "/usr/bin/id",
    "/usr/bin/join",
    "/usr/bin/link",
    "/usr/bin/logname",
    "/usr/bin/md5sum",
    "/usr/bin/mkfifo",
    "/usr/bin/nl",
    "/usr/bin/nproc",
    "/usr/bin/nohup",
    "/usr/bin/od",
    "/usr/bin/paste",
    "/usr/bin/pathchk",
    "/usr/bin/pr",
    "/usr/bin/printenv",
    "/usr/bin/printf",
    "/usr/bin/ptx",
    "/usr/bin/runcon",
    "/usr/bin/seq",
    "/usr/bin/sha1sum",
    "/usr/bin/sha224sum",
    "/usr/bin/sha256sum",
    "/usr/bin/sha384sum",
    "/usr/bin/sha512sum",
    "/usr/bin/shred",
    "/usr/bin/shuf",
    "/usr/bin/sort",
    "/usr/bin/split",
    "/usr/bin/stat",
    "/usr/bin/sum",
    "/usr/bin/tac",
    "/usr/bin/tail",
    "/usr/bin/tee",
    "/usr/bin/test",
    "/usr/bin/timeout",
    "/usr/bin/tr",
    "/usr/bin/truncate",
    "/usr/bin/tsort",
    "/usr/bin/tty",
    "/usr/bin/unexpand",
    "/usr/bin/uniq",
    "/usr/bin/unlink",
    "/usr/bin/wc",
    "/usr/bin/whoami",
    "/usr/bin/yes",
    "/usr/bin/arch",
    "/usr/bin/touch",
    "/usr/bin/md5sum.textutils",


// TODO: we need to do this automatically somehow:
    "/home/ondrej/",
};
#define Nallowed sizeof(allowed) / sizeof(char*)

// Forbid silently
char *ignore[] = {
    "/usr/local/include/",
    "/etc/passwd",
    "/etc/group",
};
#define Nignore sizeof(ignore) / sizeof(char*)

// Forbid and report
char *forbidden[] = {
    "/usr",
};
#define Nforbidden sizeof(forbidden) / sizeof(char*)

void expand_path(const char *pathname, char fullpath[PATH_MAX])
{
    if (!realpath(pathname, fullpath)) {
        fullpath = "";
    }
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
    char fullpath[PATH_MAX];
    if (logfd == -1) {
        _open = dlsym(RTLD_NEXT, "open");
        logfd = _open("/tmp/access.log", O_WRONLY|O_APPEND|O_CREAT, 0666);
    }
    expand_path(pathname, fullpath);
#ifdef DEBUG
    if (logfd != -1) {
        dprintf(logfd, "Checking file: %s\n", fullpath);
    }
#endif
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
        dprintf(logfd, "RESTRICT: untracked file: %s\n", fullpath);
    }
    return 0;
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
