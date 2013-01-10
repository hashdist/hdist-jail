# Strategy of test suite is to, for each test, compile a small C program
# from a string, run it, and inspect the results. stdout is captured
# from the programs and returned.

import os
import sys
from os.path import join as pjoin
import subprocess
import tempfile
import contextlib
import shutil
from textwrap import dedent
import errno
import functools
import nose
from nose.tools import ok_, eq_
from glob import glob

JAIL_SO = os.path.realpath(pjoin(os.path.dirname(__file__), 'build', 'hdistjail.so'))

#
# Fixture/utils
#

@contextlib.contextmanager
def temp_dir(keep=False, prefix='tmp'):
    tempdir = tempfile.mkdtemp(prefix=prefix)
    try:
        yield tempdir
    finally:
        if not keep:
            shutil.rmtree(tempdir)


def fixture():
    def decorator(func):
        @functools.wraps(func)
        def decorated():
            tempdir = tempfile.mkdtemp(prefix='jailtest-')
            try:
                os.mkdir(pjoin(tempdir, 'work'))
                return func(tempdir)
            finally:
                shutil.rmtree(tempdir)
        return decorated
    return decorator

def mock_files(tempdir, filenames):
    for filename in filenames:
        p = pjoin(tempdir, 'work', filename)
        try:
            os.makedirs(os.path.dirname(p))
        except OSError, e:
            if e.errno == errno.EEXIST:
                pass
        with file(p, 'w') as f:
            f.write('contents\n')

def compile(path, main_func_code):
    with file(path + '.c', 'w') as f:
        f.write(dedent('''
        #include <stdio.h>
        #include <stdlib.h>
        #include <sys/types.h>
        #include <sys/stat.h>
        #include <unistd.h>
        #include <fcntl.h>
        #include <errno.h>

        int main() {
        %s
        return 0;
        }
        ''') % main_func_code)
    subprocess.check_call(['gcc', '-O0', '-g', '-o', path, path + '.c'])

def run_in_jail(tempdir,
                main_func_code,
                jail_mode=None,
                whitelist=None,
                stderr=False,
                should_log=True):
    work_dir = pjoin(tempdir, 'work')
    executable = pjoin(tempdir, 'test')
    compile(executable, dedent(main_func_code))
    cmd = [executable]
    env = dict(LD_PRELOAD=JAIL_SO)
    if jail_mode:
        env['HDIST_JAIL_MODE'] = jail_mode
    if should_log:
        log_filename = pjoin(tempdir, 'log')
        env['HDIST_JAIL_LOG'] = log_filename
    if stderr:
        env['HDIST_JAIL_STDERR'] = 'hdistjail: '

    if whitelist is not None:
        # make whitelist contain absolute paths
        whitelist = [
            x if os.path.isabs(x) else pjoin(work_dir, x)
            for x in whitelist]
        whitelist_path = pjoin(tempdir, 'whitelist.txt')
        with file(whitelist_path, 'w') as f:
            f.write('\n'.join(whitelist) + '\n')
        env['HDIST_JAIL_WHITELIST'] = whitelist_path

    # make a bash script too in case manual debugging is needed
    run_sh = pjoin(tempdir, 'run.sh')
    with file(run_sh, 'w') as f:
        f.write('#!/bin/bash\n')
        f.write('set -e\n')
        for key, var in env.items():
            f.write('export %s="%s"\n' % (key, var))
        f.write('(cd work; ../test)\n')
        os.chmod(run_sh, 0777)

    # ...but run the test in a slightly more controlled environment
    proc = subprocess.Popen(cmd, cwd=work_dir, env=env,
                            stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE)
    out, err = proc.communicate()
    ret = proc.wait()
    if ret != 0:
        raise subprocess.CalledProcessError(cmd, ret)
    lines = [x for x in out.splitlines() if x]
    if should_log:
        with file(log_filename) as f:
            log_lines = [x[:-1].split(' ', 1) for x in f.readlines()]
        os.unlink(log_filename)
        assert all(int(tup[0]) == proc.pid for tup in log_lines)
        log = [tup[1] for tup in log_lines]
    else:
        log = None
    return log, lines

def run_int_checks(tempdir, preamble, checks, **kw):
    """like run_in_jail, but takes a list of integer-producing statements
    and converts the output to ints"""
    code = preamble + '\n'
    code += '\n'.join(r'printf("%%d\n", (%s));' % check for check in checks)
    log, lines = run_in_jail(tempdir, code, **kw)
    return log, [int(x) for x in lines]

def run_assertions(tempdir, preamble, checks, **kw):
    check_stats = [tup[0] for tup in checks]
    check_log = [tup[1] for tup in checks]
    log, lines = run_int_checks(tempdir, preamble, check_stats, **kw)
    for got_log, got_line, (expr, want_log) in zip(log, lines, checks):
        if got_line != 1:
            raise AssertionError('assertion failed: %s' % expr)
        assert got_log.startswith(tempdir + '/work/')
        got_log = got_log[len(tempdir + '/work/'):]
        eq_(want_log, got_log)

#
# Tests
#

@fixture()
def test_test_machinery(tempdir):
    log, out = run_in_jail(tempdir, r'''
       printf("Hello %s\n", getenv("LD_PRELOAD"));
       ''')
    eq_(['Hello %s' % JAIL_SO], out)

@fixture()
def test_whitelist_behaviour(tempdir):
    mock_files(tempdir, ['okfile', 'hiddenfile', 'subdir/foo', 'subdir/bar'])
    checks = ['open("okfile", O_RDONLY) != -1',
              'open("hiddenfile", O_RDONLY) != -1',
              'errno',
              'open("subdir/foo", O_RDONLY) != -1'
              ]
    # jail_mode='hide'
    log, out = run_int_checks(tempdir, '', checks, jail_mode='hide',
                              whitelist=['okfile', 'subdir/**'])
    eq_([1, 0, errno.ENOENT, 1], out)
    eq_(['%s/work/hiddenfile// open' % tempdir], log)
    # jail_mode='off'
    log, out = run_int_checks(tempdir, '', checks, jail_mode='off',
                              whitelist=['okfile', 'subdir/**'])
    eq_([1, 1, 0, 1], out)
    eq_(['%s/work/hiddenfile// open' % tempdir], log)

@fixture()
def test_log_no_whitelist(tempdir):
    log, out = run_int_checks(
        tempdir,
        '',
        ['open("logged", O_RDONLY) != -1'])

@fixture()
def test_simple_funcs(tempdir):
    # simply checks that functions still work and that the access is logged;
    # the whitelisting is checked
    # above (for the open call only, but that exersizes teh call)
    mock_files(tempdir, ["okfile"])
    run_assertions(tempdir, 'struct stat s;',
                   [('fopen("okfile", "r") != NULL', 'okfile// fopen'),
                    ('fopen("notpresent", "r") == NULL', 'notpresent// fopen'),
                    ('fopen("created", "w") != NULL', 'created// fopen'),
                    ('access("okfile", R_OK) == 0', 'okfile// access'),
                    ('access("notpresent", R_OK) == -1', 'notpresent// access'),
                    ('stat("okfile", &s) == 0', 'okfile// stat')])

@fixture()
def test_open(tempdir):
    mock_files(tempdir, ["okfile"])
    1/0 # make sure to test both with vararg and without...
    run_assertions(tempdir, '',
                   [('open("okfile", "r") != NULL', 'okfile// fopen'),
                    ('fopen("notpresent", "r") == NULL', 'notpresent// fopen'),
                    ('fopen("created", "w") != NULL', 'created// fopen'),
                    ('access("okfile", R_OK) == 0', 'okfile// access'),
                    ('access("notpresent", R_OK) == -1', 'notpresent// access'),
                    ('stat("okfile", &s) == 0', 'okfile// stat')])
    
   

if __name__ == '__main__':
    nose.main(sys.argv)

