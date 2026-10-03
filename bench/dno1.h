/*
 * SPDX-License-Identifier: MIT
 *
 * Portable "DoNotOptimize" helpers for micro-benchmarks.
 *
 * Goal: make the compiler assume a value is *used* and memory may have been
 * read/modified, so that the computation producing it cannot be removed,
 * hoisted or constant-folded (e.g. under LTO), without adding memory traffic
 * to the measured loop.
 *
 *   do_not_optimize(p)            Pointer escapes; pointee may be read/written.
 *   clobber_memory()              Compiler barrier: all memory may have changed.
 *   DO_NOT_OPTIMIZE_SCALAR(x)     Integer/pointer lvalue is "used and modified".
 *                                 GCC/Clang: register-only, zero memory ops.
 *   DO_NOT_OPTIMIZE(expr)         Any expression (needs typeof: GNU or C23).
 */
#pragma once

#if defined(__GNUC__) || defined(__clang__)

static inline void
do_not_optimize(void *p)
{
    /* The asm "uses" p and, via the clobber, may touch any memory. */
    __asm__ __volatile__("" : : "g"(p) : "memory");
}

static inline void
clobber_memory(void)
{
    __asm__ __volatile__("" : : : "memory");
}

/* Only for types that fit a general register (integers, pointers). */
#define DO_NOT_OPTIMIZE_SCALAR(x) __asm__ __volatile__("" : "+r"(x))

#elif defined(_MSC_VER)

#include <intrin.h>
#pragma intrinsic(_ReadWriteBarrier)

static inline void
do_not_optimize(void *p)
{
    static void *volatile sink;
    sink = p;               /* volatile store: p must be materialized */
    _ReadWriteBarrier();    /* compiler barrier (MSVC has no inline asm on x64) */
}

static inline void
clobber_memory(void)
{
    _ReadWriteBarrier();
}

#define DO_NOT_OPTIMIZE_SCALAR(x) do_not_optimize((void *)&(x))

#else /* Unknown compiler: best-effort fallback */

static inline void
do_not_optimize(void *p)
{
    static void *volatile sink;
    sink = p;
}

static inline void
clobber_memory(void)
{
    static volatile int sink;
    sink = 0;
}

#define DO_NOT_OPTIMIZE_SCALAR(x) do_not_optimize((void *)&(x))

#endif

#if defined(__GNUC__) || defined(__clang__) || \
    (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#    define DO_NOT_OPTIMIZE(expr) \
        do { typeof(expr) v_ = (expr); do_not_optimize(&v_); } while (0)
#  else
#    define DO_NOT_OPTIMIZE(expr) \
        do { __typeof__(expr) v_ = (expr); do_not_optimize(&v_); } while (0)
#  endif
#endif
