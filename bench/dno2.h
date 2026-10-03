#ifndef DO_NOT_OPTIMIZE_H
#define DO_NOT_OPTIMIZE_H

/* ============================================================
 * Compiler Detection
 * ============================================================ */

#if defined(_MSC_VER)
    #define DNO_USE_MSVC
#elif defined(__clang__) || defined(__GNUC__) || defined(__INTEL_COMPILER)
    #define DNO_USE_GCC_ASM
#else
    #define DNO_USE_FALLBACK
#endif

/* ============================================================
 * Main Macro Implementation
 * ============================================================ */

#ifdef DNO_USE_GCC_ASM

/* GCC/Clang/Intel - inline assembly barrier */
#define DoNotOptimize(val) \
    do { \
        __asm__ volatile("" : : "r,m"(val) : "memory"); \
    } while(0)

#define ClobberMemory() \
    do { \
        __asm__ volatile("" : : : "memory"); \
    } while(0)

#elif defined(DNO_USE_MSVC)

/* MSVC - use volatile pointer trick + intrinsics */
#include <intrin.h>

#define DoNotOptimize(val) \
    do { \
        volatile char* __dno_p = (volatile char*)&(val); \
        __dno_p[0] = __dno_p[0]; \
        __faststorefence(); \
    } while(0)

#define ClobberMemory() \
    do { \
        __faststorefence(); \
    } while(0)

#else

/* Unknown compiler - conservative fallback */
#define DoNotOptimize(val) \
    do { \
        volatile void* __dno_p = &(val); \
        (void)__dno_p; \
    } while(0)

#define ClobberMemory() \
    do { \
        /* No reliable barrier available */ \
    } while(0)

#endif

/* ============================================================
 * Type-Specific Helper Functions (C-only)
 * ============================================================ */

#ifdef DNO_USE_GCC_ASM

static inline void DoNotOptimizeInt(int v) {
    asm volatile("" : : "r,m"(v) : "memory");
}

static inline void DoNotOptimizeLong(long v) {
    asm volatile("" : : "r,m"(v) : "memory");
}

static inline void DoNotOptimizePtr(void* v) {
    asm volatile("" : : "r,m"(v) : "memory");
}

#elif defined(DNO_USE_MSVC)

static inline void DoNotOptimizeInt(int v) {
    volatile int* p = &v;
    *p = *p;
    __faststorefence();
}

static inline void DoNotOptimizeLong(long v) {
    volatile long* p = &v;
    *p = *p;
    __faststorefence();
}

static inline void DoNotOptimizePtr(void* v) {
    volatile void** p = &v;
    *p = *p;
    __faststorefence();
}

#else

static inline void DoNotOptimizeInt(int v) {
    volatile int* p = &v;
    (void)p;
}

static inline void DoNotOptimizeLong(long v) {
    volatile long* p = &v;
    (void)p;
}

static inline void DoNotOptimizePtr(void* v) {
    volatile void** p = &v;
    (void)p;
}

#endif

#endif /* DO_NOT_OPTIMIZE_H */
