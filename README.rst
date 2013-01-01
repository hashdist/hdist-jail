hdistrestrict
=============

The restrict library restricts the ``open()``, ``open64()`` and ``fopen()``
calls to only allow opening "allowed" files. Compile with::

    ./compile

Examples of usage::

    LD_PRELOAD=./restrict.so touch test_file
    LD_PRELOAD=./restrict.so cat test_file

How it Works
------------

It preloads the ``open()``, ``open64()`` and ``fopen()`` libc calls using the
``LD_PRELOAD`` mechanism. This means that static binaries, as well as accessing
files by other means than through ``open/open64/fopen`` will not be checked.

Copyright/license
-----------------

Copyright (c) 2012, Dag Sverre Seljebotn and Ondrej Certik.

All rights reserved.

hdistrestrict is licensed under the BSD 3-clause license. See LICENSE.txt
for full details.
