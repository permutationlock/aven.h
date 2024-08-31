# libaven: a small portable C library for slices, results, arenas, and more

I love programming in C, but I almost always need slices, optionals, and
result types (a.k.a. "errors as values").
The `aven.h` header contains a few minimal and unintrusive definitions.
It also defines a debugger friendly `assert` macro and bounds checked slice
access macros.

In total library includes:

 - slices, optionals, and results: `aven.h`
 - arena allocation: `aven/arena.h` ([inspired by this post][2])
 - slice based strings: `aven/string.h`
 - command line argument parsing: `aven/arg.h`
 - a C build system: `aven/build.h`, `aven/build/common.h`
 - a tiny SIMD linear algebra library: `aven/glm.h`
 - portable shared library loading: `aven/dl.h`
 - portable directory watching: `aven/watch.h`

Everything is cross-platform (POSIX[^1] and Windows). 

## Minimizing namespace polution

All identifiers for functions, variables, and macros will be in snake case
and begin with a prefix for the corresponding header path, except for those
defined in `aven.h`. E.g. the allocate
function defined in `aven/arena.h` is `aven_arena_alloc`.

When built as a separate translation unit using the build system (see below),
the headers will only include the following C standard headers:
`stddef.h`, `stdbool.h`, `stdint.h`, and `stdassert.h`.
If compiling for C11 then `stdalign.h` and `stdnoreturn.h` are also included.
If using the standalone `aven/time.h` portable timing header, then the C std
lib's `time.h` is included for `timespec` support.

When used as a header only library (by defining the `AVEN_IMPLEMENTATION` macro)
a small number of other C standard library headers will be included.
For POSIX targets, a few POSIX specific headers will be included.
For Windows targets, bespoke definitions are used in lieu of
any Windows or MinGW POSIX specific headers.

## Aven C build system

The most prominent part of the library is the build system. I've long been
frustrated by how non-portable Makefiles are, and how complicated larger build
systems are. I wanted my build system to satisfy the following requirements:

 - it should depend only on the existence of a C compiler toolchain:
   a C compiler (`cc`), an archiver (`ar`), and a linker (`cc` or a separate
   `ld`);
 - it should include a portable API to interact with the filesystem
   (`mkdir`, `rm`, `rmdir`, `touch`) wihtout relying on external binaries[^2];
 - build scripts should describe steps (evokations of the above tools/actions)
   and the dependencies between steps;
 - the user must be able to specify exactly what executables and flags will
   be used for each build tool, e.g. how the variables `CC` and `CFLAGS` are
   used in Makefiles;
 - there must by a standard easy way for a parent project to build and use
   artifacts from a dependency project.

The build system is
designed to support as many C compiler toolchains as possible.

The following toochains are supported by default, e.g. no configuration flags
are required for running the `build` executable to build targetting the host
system. The flags are not necessary and only shown to indicate the default
tools.

 - GNU (Linux/POSIX): -cc `gcc` -ar `ar`
 - GNU (Windows via [MinGW][3]): -cc `gcc.exe` -ar `ar.exe`
 - MSVC (Windows): -cc `cl.exe` -ld `link.ex` -ar `lib.exe`

The following toolchains should be supported with the indicated
minor configuration adjustments.

 - [Zig][1]: -cc `zig` -ccflags "cc" -ldflags "cc" -ar `zig` -arflags "ar -rcs"
 - [tinycc][5] (only tested on LINUX): -cc `tcc` -ccflags
   "-D\_\_BIGGEST\_ALIGNMENT\_\_=16" -ar `tcc` -arflags "-ar -rcs"
 - [cproc][4] + GNU (POSIX only): -cc `cproc` -ccflags "-std=c11" -ar `ar`

And hopefully other toolchains are supported as well! The MSVC
toolchain is weird as hell with how it takes arguments, and thus
a lot of accommodating compatibility options are available.

## Building the library

A static library can built using the contained build system.  

### Building the build system

On POSIX systems you can build the build system by simply running

```shell
make
```

You can also just compile it with your favorite C compiler,
e.g. using [tinycc] on Linux

```shell
tcc -D__BIGGEST_ALIGNMENT__=16 -o build build.c
```

or MSVC on Windows

```shell
cl /Fe:build.exe build.c
```

### Showing the build system help message

```shell
./build help
```

### Building the library

```shell
./build
```

### Running the tests

```shell
./build test
```

### Cleaning up the build artifacts

```shell
./build clean
```

[^1]: Finding the path to a running executable is not standard even for
    POSIX systems. Currently `aven_path_exe` is implemented
    for Windows and Linux only.

[^2]: If you have ever attempted to implement a `make clean` step that works
    on Linux and Windows, then you know what I am talking about :(

[1]: https://ziglang.org/
[2]: https://nullprogram.com/blog/2023/09/27/
[3]: https://www.mingw-w64.org/
[4]: https://sr.ht/~mcf/cproc/
[5]: https://repo.or.cz/w/tinycc.git
