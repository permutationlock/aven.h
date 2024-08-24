#ifndef AVEN_H
#define AVEN_H

#include <iso646.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Inspired by and/or copied from Chris Wellons (https://nullprogram.com)

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#ifndef __has_attribute
    #define __has_attribute(unused) 0
#endif

#ifndef __has_builtin
    #define __has_builtin(unused) 0
#endif

#if defined(AVEN_UNREACHABLE_ASSERT) and __has_builtin(__builtin_unreachable)
    #ifndef NDEBUG
        #define assert(c) while (!(c)) { __builtin_unreachable(); }
    #else
        #define assert(unused) ((void)0)
    #endif
#else
    #include <assert.h>
#endif

#define countof(array) (sizeof(array) / sizeof(*array))

#if __STDC_VERSION__ >= 201112L
    #include <stdnoreturn.h>
#elif __STDC_VERSION__ >= 199901L
    #define noreturn
#else
    #error "C99 or later is required"
#endif

#define  Optional(t) struct { t value; bool valid; }
#define Result(t) struct { t payload; int error; }
#define Slice(t) struct { t *ptr; size_t len; }
#define List(t, n) struct { size_t len; t array[n]; }

typedef Slice(unsigned char) ByteSlice;

#ifndef NDEBUG
    static inline size_t aven_assert_lt_internal_fn(size_t a, size_t b) {
        assert(a < b);
        return a;
    }

    #define aven_assert_lt_internal(a, b) aven_assert_lt_internal_fn(a, b)
#else
    #define aven_assert_lt_internal(a, b) a
#endif

#define slice_get(s, i) s.ptr[aven_assert_lt_internal(i, s.len)]
#define list_get(l, i) l.array[aven_assert_lt_internal(i, l.len)]

#define as_bytes(ptr) (ByteSlice){ \
        .ptr = (unsigned char *)ptr, \
        .len = sizeof(*ptr) \
    }
#define array_as_bytes(ptr) (ByteSlice){ \
        .ptr = (unsigned char *)ptr, \
        .len = sizeof(ptr)\
    }
#define slice_as_bytes(s) (ByteSlice){ \
        .ptr = (unsigned char *)s.ptr, \
        .len = s.len * sizeof(*s.ptr), \
    }

void *memcpy(void *restrict s1, const void *restrict s2, size_t n);

#define slice_copy(d, s) memcpy( \
        d.ptr, \
        s.ptr, \
        aven_assert_lt_internal( \
            s.len * sizeof(*s.ptr), \
            d.len * sizeof(*d.ptr) + 1 \
        ) \
    )

#endif // AVEN_H
