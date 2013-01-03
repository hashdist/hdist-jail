#include "abspath.h"

#include <stdio.h>
#include <stdlib.h>

void test(char *s) {
    char *abs_s = abspath(s);
    char *norm_s = strdup(s);
    normpath(norm_s);
    printf("%-25s -> '%s'  '%s'\n", s, abs_s, norm_s);
    free(abs_s);
    free(norm_s);
}

int main(int argc, char *argv[]) {
    printf("\n");
    test("foo////bar//.//.././x");
    test("foo////bar//.//.././x/");
    test(".");
    test("foo/.");
    test("foo/bar/..");
    test("foo/bar/../");
    test("foo/bar/../..");
    test("foo/bar/../../");
    test("..");
    test("../../../../../../..");
    test("../../../../../../../");
    test("/../../../../../../..");
    test("/../../../../../../../");
    test("/..");
    test("/../");
    test("/");
    test("/.");
    test("/.///././././etc");
    test("/.///././.././etc");
    return 0;
}
