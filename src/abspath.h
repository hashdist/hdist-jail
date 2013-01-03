#ifndef _31ba7e8a_b70a_4c29_a7c5_4c6dd2b367fe
#define _31ba7e8a_b70a_4c29_a7c5_4c6dd2b367fe

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/*
getcwd() into a newly allocated buffer; the buffer
has room for `extra` more bytes
 */
static char *alloc_getcwd(size_t extra) {
    size_t n = 256;
    char *r;
    while (1) {
        char *buf = malloc(n + extra);
        if (getcwd(buf, n)) {
            return buf;
        }
        free(buf);
        if (errno != ERANGE) return NULL;
        n *= 2;
    }
}

/* normpath: normalize a path name by translating

       "//" -> "/"
       "/./" -> "/"
       "/x/../" -> "/x/"

   Also remove a single trailing /

   No interaction with the filesystem is done. The translation
   happens in-place, since the output string will never be longer
   than the input.

   Extra '..' that cannot move to parent are skipped. This makes
   sense for absolute paths since '/..' -> '/', but is sort of nonsensical
   for relative paths.
 */
static void normpath(char *p) {
    /* note: cannot use strcat, strcpy as buffers overlap */
    char *start = p;
    int is_abs = p[0] == '/';

    /* special cases for when p is relative and starts with '..' */
    while (p[0] == '.' && p[1] == '.') {
        if (p[2] == 0) {
            p[0] = 0;
            return;
        } else if (p[2] == '/') {
            char *dst = p, *src = p + 3;
            while (*dst++ = *src++);
        } else {
            break;
        }
    }

    while (1) {
        while (*p != 0 && *p != '/') ++p;
        if (*p == 0) break;

        /* invariant: p[0] == '/' */
        if (p[1] == '/') {
            /* // -> / */
            char *dst = p + 1, *src = p + 2;
            while (*dst++ = *src++);
        } else if (p[1] == '.' && p[2] == '/') {
            /* "/./" to "/." to "/" */
            char *dst = p + 1, *src = p + 3;
            while (*dst++ = *src++);
        } else if (p[1] == '.' && p[2] == 0) {
            /* trailing "/." */
            p[0] = 0;
        } else if (p[1] == '.' && p[2] == '.' && (p[3] == '/' || p[3] == 0)) {
            /* backtrace to handle '..' */
            char *rest = p + 3;
            if (p != start) --p;
            while  (*p != '/' && p != start) --p;
            if (p == start && start[0] == '/' && rest[0] == 0) {
                /* `rest` does not start with '/' but we need to preserve  */
                start[1] = 0;
                return;
            } else {
                char *dst = p;
                while (*dst++ = *rest++);
            }
        } else {
            /* no folding has taken place, so we skip to next component;
               (otherwise repeat in current position and see if we can fold more) */
            p++;
        }
    }

    /* remove trailing /, unless it is the starting / */
    if (p != start && !is_abs && p[-1] == '/') p[-1] = 0;
}


/* returns the absolute path of `s` *without* resolving symlinks;
   done by concatenating the cwd with s (if it is not absolute already)
   and then calling normpath. The result must be free()d.

   If the path contains too many '..', so that one "escapes the root",
   NULL is returned.
*/
static char *abspath(const char *s) {
    char *path;
    if (s[0] != '/') {
        path = alloc_getcwd(strlen(s) + 1);
        strcat(path, "/");
        strcat(path, s);
    } else {
        path = strdup(s);
    }
    normpath(path);
    return path;
}

#endif

