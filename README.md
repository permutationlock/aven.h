# libaven: a tiny portable C library and build system

I love programming in C, but I always need slices, optionals, and
result types (a.k.a. "errors as values").
The `aven.h` header contains unintrusive definitions for these types.
It also defines a debugger friendly `assert` macro and bounds checked slice
access macros.

The library has expanded to include:

 - slices, optionals, and results: `aven.h`
 - arena allocation: `aven/arena.h` ([inspired by this post][2])
 - command line argument parsing: `aven/arg.h`
 - a C build system: `aven/build.h`, `aven/build/common.h`
 - portable file system interaction: `aven/fs.h`
 - a tiny SIMD linear algebra library: `aven/glm.h`
 - portable process execution and management: `aven/proc.h`
 - slice based strings: `aven/string.h`
 - a bare-bones test framework: `aven/test.h`
 - portable high precision timing: `aven/time.h`
 - portable directory watching: `aven/watch.h`

Everything is cross-platform (POSIX[^1] and Windows). 

## Minimizing namespace polution

All identifiers for functions, variables, and macros are snake case
and begin with a prefix for the corresponding header path, except for the
core defined in `aven.h`. E.g. the alloc function defined in `aven/arena.h` is
`aven_arena_alloc`.

When built as a separate translation unit using the build system (see below),
the headers will only include the following C standard headers:
`stddef.h`, `stdbool.h`, `stdint.h`, and `stdassert.h`.
If compiling for C11 then `stdalign.h` and `stdnoreturn.h` are also included.
If using the standalone `aven/time.h` portable timing header, then the libc
`time.h` is included for `timespec` support.

When used as a header only library via the `AVEN_IMPLEMENTATION` macro,
a small number of other C standard library headers will be included.
For Windows targets, bespoke definitions are used in lieu of
any Windows, MSVC, or MinGW platform specific headers.
For POSIX targets, some files require POSIX features to be enabled via
the `_POSIX_C_SOURCE` macro, and some POSIX specific headers may be included,
e.g. `fcntl.h`, `sys/stat.h`, `sys/wait.h`, and `unistd.h` [^3].

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

The following toochains are fully supported, e.g. configuration
defaults will work out-of-the-box when one is used to compile the `build.c`. 

 - GNU (POSIX + Windows w/[MinGW][3]): -cc `gcc` -ar `ar`
 - clang (POSIX + Windows w/MSVC or MinGW): -cc `clang` -ar `llvm-ar`
 - MSVC (Windows): -cc `cl.exe` -ld `link.ex` -ar `lib.exe`
 - [tinycc][5] (POSIX + Windows): -cc `tcc` -ccflags
   "-D\_\_BIGGEST\_ALIGNMENT\_\_=16" -ar `tcc` -arflags "-ar -rcs"

The following toolchains are undectectable from predefined macros, but have
been tested with the indicated configuration.

 - [Zig][1] (POSIX + Windows): -cc `zig` -ccflags "cc" -ldflags "cc" -ar `zig`
   -arflags "ar -rcs"
 - [cproc][4] w/GNU (POSIX): -cc `cproc` -ccflags "-std=c11" -ar `ar`

Hopefully many other toolchains are supported as well! The MSVC
toolchain is so weird that the build configuration has been expanded to be
very accommodating.

## Building the library

A static object file can built using the contained build system.  

### Building the build system

On POSIX systems you can build the build system by simply running

```shell
make
```

You can also just compile it with your favorite C compiler,
e.g. using [tinycc][5] on Linux

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

[^2]: If you have ever tried to write a `make clean` step that works
    on both Linux and Windows, then you know the motivation here :(

[^3]: I like to know what is in my C namespace. I am a [musl][6]
      man and can easliy read through the well written libc and POSIX headers.
      For targets with less "accessible" headers, I tried to take more control.

[1]: https://ziglang.org/
[2]: https://nullprogram.com/blog/2023/09/27/
[3]: https://www.mingw-w64.org/
[4]: https://sr.ht/~mcf/cproc/
[5]: https://repo.or.cz/w/tinycc.git
[6]: https://musl.libc.org/
