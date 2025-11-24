// POSIX functions declared but not defined by newlib.
// There are macros: _flockfile and _funlockfile in <sys/stdio.h>, which the implementation below
// is based on.

// Proper fix would be for dkP newlib to link copy and link against something like this file.

// See also esp-idf issue ticket: https://github.com/espressif/esp-idf/issues/2565

// Not sure why upstream newlib does all these hoops (provide declaration, but avoid providing a
// definition, despite using the functionality internally)...

// Not having this file causes recent versions of fmtlib to fail to link, and will cause the next
// release of libstdc++ to fail linking as well.

#include <stdio.h>
#include <sys/stdio.h>

void flockfile(FILE *f) {
    if (!(f->_flags & __SSTR)) {
        __lock_acquire_recursive(f->_lock);
    }
}

void funlockfile(FILE *f) {
    if (!(f->_flags & __SSTR)) {
        __lock_release_recursive(f->_lock);
    }
}

int ftrylockfile(FILE *f) {
    if (!(f->_flags & __SSTR)) {
        return __lock_try_acquire_recursive(f->_lock);
    }

    return 0;
}
