Restrict
========

The restrict library restricts the open(2) call to only allow opening "allowed"
files. Compile with::

    ./compile

Examples of usage::

    LD_PRELOAD=./restrict.so touch test_file
    LD_PRELOAD=./restrict.so cat test_file
