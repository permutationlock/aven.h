#ifndef AVEN_GLM_H
#define AVEN_GLM_H

#include "../aven.h"

#define AVEN_GLM_PI_D 3.14159265358979323846264338327950288
#define AVEN_GLM_PI_F 3.14159265358979323846264338327950288f

#if ( \
    defined(__GNUC__) && \
    __has_attribute(vector_size) && \
    __has_attribute(aligned) && \
    __has_builtin(__builtin_shuffle) \
)
    #define AVEN_GLM_SIMD
    typedef float Vec2SIMD __attribute__((vector_size(8)));
    typedef float Vec4SIMD __attribute__((vector_size(16)));

    typedef int32_t Vec2SIMDMask __attribute__((vector_size(8)));
    typedef int32_t Vec4SIMDMask __attribute__((vector_size(16)));

    typedef float Vec2[2] __attribute__((aligned(8)));
    typedef float Vec4[4] __attribute__((aligned(16)));
    typedef Vec2 Mat2[2] __attribute__((aligned(16)));
#else
    typedef float Vec2[2];
    typedef float Vec4[4];
    typedef Vec2 Mat2[2];
#endif

typedef float Vec3[3];
typedef Vec3 Mat3[3];
typedef Vec4 Mat4[4];

static inline void vec2_set(Vec2 dest, Vec2 a) {
#ifdef AVEN_GLM_SIMD
    *(Vec2SIMD *)dest = *(Vec2SIMD *)a;
#else
    dest[0] = a[0];
    dest[1] = a[1];
#endif // AVEN_GLM_SIMD
}

static inline void vec2_add(Vec2 dest, Vec2 a, Vec2 b) {
#ifdef AVEN_GLM_SIMD
    *(Vec2SIMD *)dest = *(Vec2SIMD *)a + *(Vec2SIMD *)b;
#else
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
#endif // AVEN_GLM_SIMD
}

static inline void vec2_diff(Vec2 dest, Vec2 a, Vec2 b) {
#ifdef AVEN_GLM_SIMD
    *(Vec2SIMD *)dest = *(Vec2SIMD *)a - *(Vec2SIMD *)b;
#else
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
#endif // AVEN_GLM_SIMD
}

static inline void mat2_set(Mat2 dest, Mat2 m) {
#ifdef AVEN_GLM_SIMD
    *(Vec4SIMD *)dest = *(Vec4SIMD *)m;
#else
    dest[0][0] = m[0][0];
    dest[0][1] = m[0][1];
    dest[1][0] = m[1][0];
    dest[1][1] = m[1][1];
#endif // AVEN_GLM_SIMD
}

static inline void mat2_mul_vec2(Vec2 dest, Mat2 m, Vec2 a) {
#ifdef AVEN_GLM_SIMD
    Vec4SIMD vm = *(Vec4SIMD *)m;
    Vec4SIMD va = { a[0], a[0], a[1], a[1] };
    Vec4SIMD vma = vm * va;
    Vec2SIMD vma_low = { vma[0], vma[1] };
    Vec2SIMD vma_high = { vma[2], vma[3] };
    *(Vec2SIMD *)dest = vma_low + vma_high;
#else
    dest[0] = m[0][0] * a[0] + m[1][0] * a[1];
    dest[1] = m[0][1] * a[0] + m[1][1] * a[1];
#endif // AVEN_GLM_SIMD
}

static inline void mat2_mul_mat2(Mat2 dest, Mat2 m, Mat2 n) {
#ifdef AVEN_GLM_SIMD
    Vec4SIMD vm = *(Vec4SIMD *)m;
    Vec4SIMD vn = *(Vec4SIMD *)n;

    Vec4SIMD vm0_ = __builtin_shuffle(vm, (Vec4SIMDMask){ 0, 1, 0, 1 });
    Vec4SIMD vm1_ = __builtin_shuffle(vm, (Vec4SIMDMask){ 2, 3, 2, 3 });
    Vec4SIMD vn_0 = __builtin_shuffle(vn, (Vec4SIMDMask){ 0, 0, 2, 2 });
    Vec4SIMD vn_1 = __builtin_shuffle(vn, (Vec4SIMDMask){ 1, 1, 3, 3 });
    *(Vec4SIMD *)dest = vm0_ * vn_0 + vm1_ * vn_1;
#else
    dest[0][0] = m[0][0] * n[0][0] + m[1][0] * n[0][1];
    dest[0][1] = m[0][1] * n[0][0] + m[1][1] * n[0][1];
    dest[1][0] = m[0][0] * n[1][0] + m[1][0] * n[1][1];
    dest[1][1] = m[0][1] * n[1][0] + m[1][1] * n[1][1];
#endif // AVEN_GLM_SIMD
}

static inline void vec3_set(Vec3 dest, Vec3 a) {
    dest[0] = a[0];
    dest[1] = a[1];
    dest[2] = a[2];
}

static inline void vec3_add(Vec3 dest, Vec3 a, Vec3 b) {
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
}

static inline void vec3_diff(Vec3 dest, Vec3 a, Vec3 b) {
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

static inline void mat3_set(Mat3 dest, Mat3 m) {
    memcpy(dest, m, sizeof(Mat3));
}

static inline void mat3_mul_vec3(Vec3 dest, Mat3 m, Vec3 a) {
    dest[0] = m[0][0] * a[0] + m[1][0] * a[1] + m[2][0] * a[2];
    dest[1] = m[0][1] * a[0] + m[1][1] * a[1] + m[2][1] * a[2];
    dest[2] = m[0][2] * a[0] + m[1][2] * a[1] + m[2][2] * a[2];
}

static inline void mat3_mul_mat3(Mat3 dest, Mat3 m, Mat3 n) {
    dest[0][0] = m[0][0] * n[0][0] + m[1][0] * n[0][1] + m[2][0] * n[0][2];
    dest[0][1] = m[0][1] * n[0][0] + m[1][1] * n[0][1] + m[2][1] * n[0][2];
    dest[0][2] = m[0][2] * n[0][0] + m[1][2] * n[0][1] + m[2][2] * n[0][2];
                                                                    
    dest[1][0] = m[0][0] * n[1][0] + m[1][0] * n[1][1] + m[2][0] * n[1][2];
    dest[1][1] = m[0][1] * n[1][0] + m[1][1] * n[1][1] + m[2][1] * n[1][2];
    dest[1][2] = m[0][2] * n[1][0] + m[1][2] * n[1][1] + m[2][2] * n[1][2];
                                                                    
    dest[2][0] = m[0][0] * n[2][0] + m[1][0] * n[2][1] + m[2][0] * n[2][2];
    dest[2][1] = m[0][1] * n[2][0] + m[1][1] * n[2][1] + m[2][1] * n[2][2];
    dest[2][2] = m[0][2] * n[2][0] + m[1][2] * n[2][1] + m[2][2] * n[2][2];
}

static inline void vec4_set(Vec4 dest, Vec4 a) {
#ifdef AVEN_GLM_SIMD
    *(Vec4SIMD *)dest = *(Vec4SIMD *)a;
#else
    dest[0] = a[0];
    dest[1] = a[1];
    dest[2] = a[2];
    dest[3] = a[3];
#endif // AVEN_GLM_SIMD
}

static inline void vec4_add(Vec4 dest, Vec4 a, Vec4 b) {
#ifdef AVEN_GLM_SIMD
    *(Vec4SIMD *)dest = *(Vec4SIMD *)a + *(Vec4SIMD *)b;
#else
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
    dest[3] = a[3] + b[3];
#endif // AVEN_GLM_SIMD
}

static inline void vec4_diff(Vec4 dest, Vec4 a, Vec4 b) {
#ifdef AVEN_GLM_SIMD
    *(Vec4SIMD *)dest = *(Vec4SIMD *)a - *(Vec4SIMD *)b;
#else
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
    dest[3] = a[3] - b[3];
#endif // AVEN_GLM_SIMD
}

static inline void mat4_set(Mat4 dest, Mat4 m) {
#ifdef AVEN_GLM_SIMD
    *(Vec4SIMD *)dest[0] = *(Vec4SIMD *)m[0];
    *(Vec4SIMD *)dest[1] = *(Vec4SIMD *)m[1];
    *(Vec4SIMD *)dest[2] = *(Vec4SIMD *)m[2];
    *(Vec4SIMD *)dest[3] = *(Vec4SIMD *)m[3];
#else
    memcpy(dest, m, sizeof(Mat4));
#endif // AVEN_GLM_SIMD
}

static inline void mat4_mul_vec4(Vec4 dest, Mat4 m, Vec4 a) {
#ifdef AVEN_GLM_SIMD
    Vec4SIMD vm0 = *(Vec4SIMD *)m[0];
    Vec4SIMD vm1 = *(Vec4SIMD *)m[1];
    Vec4SIMD vm2 = *(Vec4SIMD *)m[2];
    Vec4SIMD vm3 = *(Vec4SIMD *)m[3];
    Vec4SIMD va = *(Vec4SIMD *)a;

    Vec4SIMD va0 = __builtin_shuffle(va, (Vec4SIMDMask){ 0, 0, 0, 0 });
    Vec4SIMD va1 = __builtin_shuffle(va, (Vec4SIMDMask){ 1, 1, 1, 1 });
    Vec4SIMD va2 = __builtin_shuffle(va, (Vec4SIMDMask){ 2, 2, 2, 2 });
    Vec4SIMD va3 = __builtin_shuffle(va, (Vec4SIMDMask){ 3, 3, 3, 3 });
    *(Vec4SIMD *)dest = vm0 * va0 + vm1 * va1 + vm2 * va2 + vm3 * va3;
#else
    dest[0] = m[0][0] * a[0] + m[1][0] * a[1] + m[2][0] * a[2] + m[3][0] * a[3];
    dest[1] = m[0][1] * a[0] + m[1][1] * a[1] + m[2][1] * a[2] + m[3][1] * a[3];
    dest[2] = m[0][2] * a[0] + m[1][2] * a[1] + m[2][2] * a[2] + m[3][2] * a[3];
    dest[3] = m[0][3] * a[0] + m[1][3] * a[1] + m[2][3] * a[2] + m[3][3] * a[3];
#endif // AVEN_GLM_SIMD
}

static inline void mat4_mul_mat4(Mat4 dest, Mat4 m, Mat4 n) {
#ifdef AVEN_GLM_SIMD
    Vec4SIMD vm0 = *(Vec4SIMD *)m[0];
    Vec4SIMD vm1 = *(Vec4SIMD *)m[1];
    Vec4SIMD vm2 = *(Vec4SIMD *)m[2];
    Vec4SIMD vm3 = *(Vec4SIMD *)m[3];

    Vec4SIMD vn0 = *(Vec4SIMD *)n[0];
    Vec4SIMD vn1 = *(Vec4SIMD *)n[1];
    Vec4SIMD vn2 = *(Vec4SIMD *)n[2];
    Vec4SIMD vn3 = *(Vec4SIMD *)n[3];

    Vec4SIMD vn00 = __builtin_shuffle(vn0, (Vec4SIMDMask){ 0, 0, 0, 0 });
    Vec4SIMD vn01 = __builtin_shuffle(vn0, (Vec4SIMDMask){ 1, 1, 1, 1 });
    Vec4SIMD vn02 = __builtin_shuffle(vn0, (Vec4SIMDMask){ 2, 2, 2, 2 });
    Vec4SIMD vn03 = __builtin_shuffle(vn0, (Vec4SIMDMask){ 3, 3, 3, 3 });

    Vec4SIMD vn10 = __builtin_shuffle(vn1, (Vec4SIMDMask){ 0, 0, 0, 0 });
    Vec4SIMD vn11 = __builtin_shuffle(vn1, (Vec4SIMDMask){ 1, 1, 1, 1 });
    Vec4SIMD vn12 = __builtin_shuffle(vn1, (Vec4SIMDMask){ 2, 2, 2, 2 });
    Vec4SIMD vn13 = __builtin_shuffle(vn1, (Vec4SIMDMask){ 3, 3, 3, 3 });

    Vec4SIMD vn20 = __builtin_shuffle(vn2, (Vec4SIMDMask){ 0, 0, 0, 0 });
    Vec4SIMD vn21 = __builtin_shuffle(vn2, (Vec4SIMDMask){ 1, 1, 1, 1 });
    Vec4SIMD vn22 = __builtin_shuffle(vn2, (Vec4SIMDMask){ 2, 2, 2, 2 });
    Vec4SIMD vn23 = __builtin_shuffle(vn2, (Vec4SIMDMask){ 3, 3, 3, 3 });

    Vec4SIMD vn30 = __builtin_shuffle(vn3, (Vec4SIMDMask){ 0, 0, 0, 0 });
    Vec4SIMD vn31 = __builtin_shuffle(vn3, (Vec4SIMDMask){ 1, 1, 1, 1 });
    Vec4SIMD vn32 = __builtin_shuffle(vn3, (Vec4SIMDMask){ 2, 2, 2, 2 });
    Vec4SIMD vn33 = __builtin_shuffle(vn3, (Vec4SIMDMask){ 3, 3, 3, 3 });

    *(Vec4SIMD *)dest[0] = vm0 * vn00 + vm1 * vn01 + vm2 * vn02 + vm3 * vn03;
    *(Vec4SIMD *)dest[1] = vm0 * vn10 + vm1 * vn11 + vm2 * vn12 + vm3 * vn13;
    *(Vec4SIMD *)dest[2] = vm0 * vn20 + vm1 * vn21 + vm2 * vn22 + vm3 * vn23;
    *(Vec4SIMD *)dest[3] = vm0 * vn30 + vm1 * vn31 + vm2 * vn32 + vm3 * vn33;
#else
    dest[0][0] = m[0][0] * n[0][0] + m[1][0] * n[0][1] + m[2][0] * n[0][2]
        + m[3][0] * n[0][3];
    dest[0][1] = m[0][1] * n[0][0] + m[1][1] * n[0][1] + m[2][1] * n[0][2]
        + m[3][1] * n[0][3];
    dest[0][2] = m[0][2] * n[0][0] + m[1][2] * n[0][1] + m[2][2] * n[0][2]
        + m[3][2] * n[0][3];
    dest[0][3] = m[0][3] * n[0][0] + m[1][3] * n[0][1] + m[2][3] * n[0][2]
        + m[3][3] * n[0][3];

    dest[1][0] = m[0][0] * n[1][0] + m[1][0] * n[1][1] + m[2][0] * n[1][2]
        + m[3][0] * n[1][3];
    dest[1][1] = m[0][1] * n[1][0] + m[1][1] * n[1][1] + m[2][1] * n[1][2]
        + m[3][1] * n[1][3];
    dest[1][2] = m[0][2] * n[1][0] + m[1][2] * n[1][1] + m[2][2] * n[1][2]
        + m[3][2] * n[1][3];
    dest[1][3] = m[0][3] * n[1][0] + m[1][3] * n[1][1] + m[2][3] * n[1][2]
        + m[3][3] * n[1][3];

    dest[2][0] = m[0][0] * n[2][0] + m[1][0] * n[2][1] + m[2][0] * n[2][2]
        + m[3][0] * n[2][3];
    dest[2][1] = m[0][1] * n[2][0] + m[1][1] * n[2][1] + m[2][1] * n[2][2]
        + m[3][1] * n[2][3];
    dest[2][2] = m[0][2] * n[2][0] + m[1][2] * n[2][1] + m[2][2] * n[2][2]
        + m[3][2] * n[2][3];
    dest[2][3] = m[0][3] * n[2][0] + m[1][3] * n[2][1] + m[2][3] * n[2][2]
        + m[3][3] * n[2][3];

    dest[3][0] = m[0][0] * n[3][0] + m[1][0] * n[3][1] + m[2][0] * n[3][2]
        + m[3][0] * n[3][3];
    dest[3][1] = m[0][1] * n[3][0] + m[1][1] * n[3][1] + m[2][1] * n[3][2]
        + m[3][1] * n[3][3];
    dest[3][2] = m[0][2] * n[3][0] + m[1][2] * n[3][1] + m[2][2] * n[3][2]
        + m[3][2] * n[3][3];
    dest[3][3] = m[0][3] * n[3][0] + m[1][3] * n[3][1] + m[2][3] * n[3][2]
        + m[3][3] * n[3][3];
#endif // AVEN_GLM_SIMD
}

#endif // AVEN_GLM_H
