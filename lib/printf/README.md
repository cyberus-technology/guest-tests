# The printf implementation

This library contains the actual printf implementation used in the
little C/C++ standard library. It does not expose the actual `printf`
functions, but only the interface to change `printf` backends.

Components that need to call `printf` and friends need to include the
`cxx` library.
