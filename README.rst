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
    

**HDIST_JAIL_HIDE**:
    If set to exactly ``1``, then non-whitelisted file accesses will
    return the error ``ENOENT`` ("No such file or directory").
    Otherwise, access will be like normal (but may be logged).

**HDIST_JAIL_LOG**:
    Non-whitelisted file access will be logged to file. The filename
    used is ``$HDIST_JAIL_LOG-$PID-XXXXXXX``, where ``XXXXXX`` is
    determined by ``mkstemp`` to make a unique filename for this
    process. See ``man mkstemp`` for permissions of the resulting file.

    Each entry in the log file will contain first the filename terminated
    by ``\0`` (so that it can include any other character), then a space,
    then the function name (e.g., ``open``), then optionally further
    information (currently none), then ``\n``.
    

Behaviour
---------

Currently the jail only targets GNU libc.

The jail generally fails fast and **terminates** the process
if something is wrong (e.g., HDIST_JAIL_WHITELIST is present
and non-empty but the file cannot be opened). Termination is
with exit code EXIT_CODE (by default 30).

The jail always takes action on the exact argument to the call.
Consider that A is a whitelisted symlink to a non-whitelisted file B.
In that case, both ``stat`` and ``lstat`` will behave as normal when
accessing B (i.e., if B is ``stat``-ed through A then it the
access is whitelisted and OK).

Thread-safety: ``write`` is used to write entire lines at the time to
the log file, which should mean that output from different threads
are serialized (see ``man flockfile``).

Copyright/license
-----------------

Copyright (c) 2012, Dag Sverre Seljebotn and Ondrej Certik.

All rights reserved.

hdistrestrict is licensed under the BSD 3-clause license. See LICENSE.txt
for full details.

Dependencies:

 * **khash.h**: Copyright (c) 2008, 2009, 2011
   by Attractive Chaos <attractor@live.co.uk>.
   Licensed under the MIT license; see header of ``src/khash.h``.
