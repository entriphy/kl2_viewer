#pragma once

#include <cglm/cglm.h>

typedef unsigned char u8;
typedef volatile u8 vu8;
typedef signed char s8;
typedef volatile s8 vs8;

typedef unsigned short u16;
typedef volatile u16 vu16;
typedef signed short s16;
typedef volatile s16 vs16;

typedef unsigned int u32;
typedef volatile u32 vu32;
typedef signed int s32;
typedef volatile s32 vs32;

typedef unsigned long long u64;
typedef volatile u64 vu64;
typedef signed long long s64;
typedef volatile s64 vs64;

#ifdef PS2_TYPE_STUBS
typedef unsigned int u128[4];
typedef volatile u128 vu128;
typedef signed int s128[4];
typedef volatile s128 vs128;
#else
typedef unsigned int u128 __attribute__((mode(TI)));
typedef volatile u128 vu128 __attribute__((mode(TI)));
typedef signed int s128 __attribute__((mode(TI)));
typedef volatile s128 vs128 __attribute__((mode(TI)));
#endif

typedef float f32;
typedef volatile float vf32;
typedef double f64;

typedef vec4  FVECTOR;
typedef mat4  FMATRIX;
typedef ivec4 IVECTOR;

typedef union {
    u128 u_u128;
    u64  u_u64[2];
    s64  u_s64[2];
    u32  u_u32[4];
    s32  u_s32[4];
    u16  u_u16[8];
    s16  u_s16[8];
    u8   u_u8[16];
    s8   u_s8[16];
    f32  u_f32[4];
} qword_uni;

typedef union {
    u64 u_u64;
    u32 u_u32[2];
    u16 u_u16[4];
    s64 u_s64;
    s32 u_s32[2];
    s16 u_s16[4];
    f32 u_f32[2];
} long_uni;

#define SETVEC(vec, x, y, z, w) (vec[0] = x, vec[1] = y, vec[2] = z, vec[3] = w)