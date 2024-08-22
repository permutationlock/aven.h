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
 - a C build system (`aven/build.h`);
 - a tiny SIMD linear algebra library (`aven/glm.h`);
 - shared library loading (`aven/dl.h`);
 - high resolution timing (`aven/time.h`);
 - directory watching (`aven/watch.h`).

Everything is cross-platform (POSIX + Windows).

## Building the example

A somewhat interesting `build.c` is included that will watch a source directory
and rebuild the project on file changes. The project in question includes an
executable and a shared library. The shared library can be hot reloaded while
the executable is running.

### Build the build system

```shell
make
```

or simply

```shell
cc -o build build.c
```

### Build the project

```shell
./build
```

or build and watch source with

```
./build watch
```

### Run

```shell
./build_out/bin/print_funmath
```

The code was inspired by the [nullprogram blog][1].

[1]: https://nullprogram.com
