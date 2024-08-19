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

The code was inspired by the [nullprogram blog][1].

[1]: https://nullprogram.com
