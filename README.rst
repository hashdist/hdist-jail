hdist-jail
==========

Restricts the ``open()``, ``open64()`` and ``fopen()`` (etc.) calls to only
allow opening "allowed" files.

Usage
-----

Compile with ``make``. The result is found in ``build/hdistjail.so``.

A process must be started with, e.g.::

    LD_PRELOAD=path/to/hdistjail.so cat somefile

Then file system access (using ``libc.so``) is filtered according to
the environment variables below. If no variables are set, nothing
happens.


**HDIST_JAIL_WHITELIST**:
    Should point to a file which, when loaded, lists files to to be
    whitelisted so that no action is taken for them. The file should
    list one full absolute filename per line. It is also allowed
    to terminate with ``**``, e.g., ``/path/to/dir/**``, to whitelist
    an entire directory (currently the ``**`` must the last path component
    and no other forms of globbing is supported).

    If this environment variable is not present or an empty string
    then all files accesses are logged/rejected.    

**HDIST_JAIL_MODE**:
    If set, must be either an empty string or ``off``, in which case
    no action is taken (except optionally logging), or ``hide``,
    in which case non-whitelisted file accesses will
    return the error ``ENOENT`` ("No such file or directory").

**HDIST_JAIL_LOG**:
    Non-whitelisted file access will be logged to the file given.  The
    file is opened in O_APPEND mode and each entry will be done with
    a single ``write`` call of size less than ``PIPE_BUF``.

**HDIST_JAIL_STDERR**:
    If set to a non-empty string, logging will happen to stderr. Each
    log line will be prefixed with the string given.

Log file format
---------------

Each entry in the log file has the form::

    pid path// action pid

The `path` is the canonical path name as described below. The canonical
path can not contain ``//``, so this is used as a terminator, since
`path` may contain both spaces and newlines. `pid` is the PID
of the process.

`action` is usually the name of the intercepted function
(e.g., ``open``, ``fopen``), but see below.


Behaviour
---------

Paths are canonicalized by ``abspath()`` in ``src/abspath.h`` (see
below).  This differs from ``realpath()`` in that it does *not* follow
symlinks, so that filters can be applied to path access through
symlinks.

Since a process may read a symlink explicitly and then access through
the physical path, if a whitelisted path is accessed through
``readlink()``, ``realpath()`` or ``canonicalize_file_name()``
then the process may attempt. This situation is warned against
using ``readlink-of-blacklisted`` (former) or
``realpath-of-blacklisted`` (latter two) `action`.

the returned result is also added to the whitelist.

**Note:** This fails to handle the case where one process resolves the
physical path, then passes the result to another process. If that is a
problem one must augment the whitelist accordingly manually.

Currently the jail only targets GNU libc.

The jail generally fails fast and **terminates** the process
if something is wrong (e.g., HDIST_JAIL_WHITELIST is present
and non-empty but the file cannot be opened). Termination is
with exit code 30 (can be changed by defining ``EXIT_CODE`` on
compilation).

The jail always takes action on the exact argument to the call.
Consider that A is a whitelisted symlink to a non-whitelisted file B.
In that case, both ``stat`` and ``lstat`` will behave as normal when
accessing B (i.e., if B is ``stat``-ed through A then it the
access is whitelisted and OK).

Thread-safety: ``write`` is used to write entire lines at the time to
the log file, which should mean that output from different threads
are serialized (see ``man flockfile``).

Bugs
----

 * The ``execvp*`` functions (when argument does not contain a slash):
   These look through PATH for the binary to execute; hdistjail does not
   replicate this behaviour but always lets such calls through as if
   they were whitelisted.

 * ``mkstemp`` and friends are not jailed

Copyright/license
-----------------

Copyright (c) 2012, Dag Sverre Seljebotn and Ondrej Certik.

All rights reserved.

hdistjail is licensed under the BSD 3-clause license. See LICENSE.txt
for full details.

Dependencies:

 * **khash.h**: Copyright (c) 2008, 2009, 2011
   by Attractive Chaos <attractor@live.co.uk>.
   Licensed under the MIT license; see header of ``src/khash.h``.
