# CWICLO

This is a support library for a particular way of writing C++ code.
The particulars of the particular way are rather involved and require
a lot of documentation, none of which has been written yet. What you
will find here is an asynchronous COM framework with some vaguely
standard-library-like support classes and algorithms. The only reason
you would consider installing it at this point is as a dependency for
one of my other projects.

## Installation

Compilation requires C++20 support; use gcc 10 or clang 11. The target
platform is Linux. It may also compile on BSD. It is unlikely to work
anywhere else, and I have no interest whatsoever in porting it there.

```sh
./configure --prefix=/usr
make check
make install
```

That's all you need to build projects dependent on it.

## Usage

As for using it for your own, the documentation for that will be
written at some point, and a full release will be made to commemorate
that momentous occasion. Until then, your options are rather scarce.

You can read the files in [test/](test/) for usage examples.
Read [documentation](https://msharov.github.io/casycom/)
and [tutorials](https://msharov.github.io/casycom/tutfwork.html)
of [casycom](https://github.com/msharov/casycom),
which is the C version of the COM framework implemented here.
Read [documentation](https://msharov.github.io/ustl/)
for [uSTL](https://github.com/msharov/ustl),
which will give you some idea of what the design principles are.

## Bugs

Report bugs on [project bugtracker](https://github.com/msharov/cwiclo/issues).
