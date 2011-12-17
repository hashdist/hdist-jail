cdef extern from *:
    ctypedef char* const_char_p "const char*"

cdef api int can_open(const_char_p pathname):
    """
    Tests if 'pathname' can be opened.

    Returns 1 if it can, 0 if it cannot.
    """
    print "can_open called:", pathname
    return 1
