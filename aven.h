#ifndef AVEN_H
#define AVEN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define assert(c) while (!(c)) { __builtin_unreachable(); }

#define countof(array) (sizeof(array) / sizeof(*array))

#define result(t) struct { t payload; int32_t error; }
#define slice(t) struct { t *ptr; size_t len; }

#define slice_get(s, i) (s.ptr[i + aven_verify_index_internal(i, s.len)])

typedef slice(int8_t) int8_slice_t;
typedef slice(uint8_t) uint8_slice_t;
typedef slice(int16_t) int16_slice_t;
typedef slice(uint16_t) uint16_slice_t;
typedef slice(int32_t) int32_slice_t;
typedef slice(uint32_t) uint32_slice_t;
typedef slice(int64_t) int64_slice_t;
typedef slice(uint64_t) uint64_slice_t;

typedef char *cstring_t;
typedef slice(cstring_t) cstring_slice_t;

typedef slice(unsigned char) byte_slice_t;

#define as_bytes(ptr) ((byte_slice_t){ \
        .ptr = (unsigned char *)ptr, \
        .len = sizeof(*ptr)\
    })
#define array_as_bytes(ptr) ((byte_slice_t){ \
        .ptr = (unsigned char *)ptr, \
        .len = sizeof(ptr)\
    })
#define cstring_as_bytes(cstr) ((byte_slice_t){ \
        .ptr = (unsigned char *)cstr, \
        .len = sizeof(cstr) - 1 \
    })

static size_t aven_verify_index_internal(size_t index, size_t len) {
    assert(index >= 0 && index < len);
    return 0;
}

#ifdef __GNUC__
__attribute((malloc, alloc_size(2), alloc_align(3)))
#endif
void *aven_alloc(byte_slice_t *arena, size_t size, size_t align);

#ifdef AVEN_H_IMPLEMENTATION

void *aven_alloc(byte_slice_t *arena, size_t size, size_t align) {
    size_t padding = -(size_t)arena->ptr & (align - 1);
    if (size > (arena->len - padding)) {
        return 0;
    }
    unsigned char *p = arena->ptr + padding;
    arena->ptr += padding + size;
    arena->len -= padding + size;
    return p;
}

#endif // AVEN_H_IMPLEMENTATION

#endif // AVEN_H
