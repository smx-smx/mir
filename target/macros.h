#pragma once

#ifdef _MSC_VER
#define __noreturn __declspec(noreturn)
#define __unused
#define offsetof(t, f) ( (int) &((t*)0)->f )

#ifdef MSVC_OLD
// no support for static_assert
#define static_assert(cond)
#define INLINE
#else
#define static_assert(cond) static_assert(cond, #cond)
#define INLINE static inline
#endif

#ifndef TDATA
#define TDATA __declspec(dllimport)
#endif

#ifndef TFUNC
#define TFUNC __declspec(dllexport)
#endif

#ifndef EXPORT
#define EXPORT __declspec(dllexport)
#endif

#else
#define __noreturn __attribute__((noreturn))
#define __unused __attribute__((unused))
#define offsetof(t, f) __builtin_offsetof(t, f)

#ifndef __cplusplus
#define static_assert(cond) _Static_assert(cond, #cond)
#endif

#ifndef TFUNC
#define TFUNC extern
#endif
#ifndef TDATA
#define TDATA extern
#endif

#ifndef EXPORT
#define EXPORT extern
#endif

#endif

#define ARRAY_SIZE(array) \
    (sizeof(array) / sizeof(array[0]))

#define MIN(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#define MAX(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })

#define CLAMP(x, low, high) \
  ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))

#define FOURCC(s) ({ \
    _Static_assert(sizeof(s) - 1 == 4, "Wrong fourcc length"); \
    s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]; \
})
