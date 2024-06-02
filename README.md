# A C header for slices, results, and arena allocation

I love programming in C, but I almost always need slices, result types, and
arena allocation.
The `aven.h` file contains a minimal and unintrusive implementation of slices
and arenas that can be easily included in any C project.
It also defines a debug friendly `assert` macro and a bounds checked slice
access macro `slice_get`.

The code is heavily inspired by and/or copied from the [nullprogram blog][1].

[1]: https://nullprogram.com
