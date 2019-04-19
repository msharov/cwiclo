
# CWICLO

This work-in-progress is a C++ merge-rewrite of
[casycom](https://github.com/msharov/casycom) and
[uSTL](https://github.com/msharov/ustl). The result is an asynchronous
COM framework with some vaguely standard-library-like classes implemented
without exceptions, allowing the executables to not link to libstdc++
or libgcc. The intended purpose is writing low-level system services in
C++. Such services may have to run before /usr/lib and libstdc++.so are
available, and so must not be linked with anything other than libc. They
should, in fact, be linked statically, once you find a libc that does
not require a megabyte to implement printf and strcpy.

Compilation requires C++17 support; use gcc 7. clang 6 also works,
but is not recommended because its output is significantly worse (15%
larger) than from gcc.

```sh
./configure && make check && make install
```

Read the files in the [test/ directory](test) for usage examples.
Additional documentation will be written when the project is more complete.

Report bugs on [project bugtracker](https://github.com/msharov/cwiclo/issues).
