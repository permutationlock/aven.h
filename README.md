# A C header for slices, results, and arena allocation

I love programming in C, but I almost always need slices, optionals, and
result types (a.k.a. "errors as values").
The `aven.h` file contains minimal and unintrusive definitions
that can be easily included in any C project.
It also defines a debugger friendly `assert` macro and bounds checked slice
access macros.

The project has expanded to include:
 - arena allocation (`aven/arena.h`);
 - slice based strings (`aven/string.h`);
 - command line argument parsing (`aven/arg.h`);
 - a C build system (`aven/build.h`, `aven/build/common.h`);
 - a tiny SIMD linear algebra library (`aven/glm.h`);
 - shared library loading (`aven/dl.h`);
 - directory watching (`aven/watch.h`).

Everything is cross-platform (POSIX[^1] + Windows). 
The coding style was inspired by the [nullprogram blog][1].

## Building the library

A static library can built using the build system from the header
only library.

### Building the build system

```shell
make
```

or simply

```shell
cc -o build build.c
```

### Showing build system help

```shell
./build help
```

### Building the library

```shell
./build
```

### Cleaning build artifacts

```shell
./build clean
```

[^1]: Finding the path to a running executable is not standard
    accross POSIX systems. Currently `aven_path_exe` is implemented
    for Windows and Linux only.

[1]: https://nullprogram.com
