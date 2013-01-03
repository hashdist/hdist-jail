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

    /* we keep initial / out of it for absolute paths, then the rest is the same */
    char *start;
    if (p[0] == '/') p++;
    start = p;

    while (1) {
        /* Strategy: Modify p in place (copying up from the rest of the string)
           when applying rules. Only if no rules were applied do we search for
           a new path component (i.e., loop is used as rail recursion).

           Invariant: p[0] is at beginning of next path component (which may
           be empty). */
        if (p[0] == 0) {
            break;
        } else if (p[0] == '/') {
            char *dst = p, *src = p + 1;
            while (*dst++ = *src++);
        } else if (p[0] == '.' && p[1] == '/') {
            char *dst = p, *src = p + 2;
            while (*dst++ = *src++);
        } else if (p[0] == '.' && p[1] == 0) {
            p[0] = 0; /* leaves trailing / to be stripped off later */
        } else if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == 0)) {
            char *rest = p + 2;
            if (p != start) p -= 2;
            while  (*p != '/' && p != start) --p;
            if (*p == '/') ++p;
            char *dst = p;
            while (*dst++ = *rest++);
        } else {
            /* skip ahead to start of next path component */
            while (*p != 0 && *p != '/') p++;
            if (*p == '/') p++;
        }
    }
    /* remove trailing / */
    if (p != start && p[-1] == '/') p[-1] = 0;
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

