# CWICLO

This work-in-progress is a C++ merge-rewrite of
[casycom](https://github.com/msharov/casycom) and
[uSTL](https://github.com/msharov/ustl) to create a lightweight
asynchronous COM framework with some vaguely standard-library-like
classes implemented without exceptions and without needing libstdc++.

Compilation requires C++17 support; use gcc 7 or clang 6.

```sh
./configure && make check && make install
```

Read the files in [test/](test/) for usage examples.
Additional documentation will be written when the project is more complete.

Report bugs on [project bugtracker](https://github.com/msharov/cwiclo/issues).
