/*
 * SPDX-License-Identifier: MIT
 *
 * Compiler escape hatches for micro-benchmarks.
 *
 * API (all usable as statements; all arguments evaluated exactly once):
 *
 *   bench_do_not_optimize(lvalue)
 *       "Sink": the value is used by something the compiler cannot see, so the
 *       computation producing it cannot be dead-code eliminated.
 *       `lvalue`: scalar or pointer (integer, bool, enum, pointer, float).
 *
 *   bench_launder(lvalue)
 *       "Opaque": the compiler must forget everything it knows about the
 *       variable's current value. Use it on *inputs* INSIDE the loop to defeat
 *       constant folding and loop-invariant hoisting:
 *           for (...) { bench_launder(n); r = f(n); bench_do_not_optimize(r); }
 *       `lvalue`: modifiable scalar or pointer.
 *
 *   bench_escape(ptr)
 *       The pointed-to object may be read AND written by unknown code.
 *       Use it for buffers, structs and arrays (anything that is not a scalar).
 *
 *   bench_clobber_memory()
 *       Compiler memory barrier: all memory may have been read/modified.
 *
 * Guarantees that hold on every supported compiler:
 *   - Misuse fails to compile instead of silently doing nothing: rvalues
 *     (e.g. `bench_do_not_optimize(f(x))`) are rejected everywhere, not only
 *     on some compilers. Assign to a local first.
 *   - Strict ISO C (-std=c99/c11/c17/c2x -pedantic) is supported; no GNU `asm`
 *     keyword is used.
 *   - Names are prefixed to avoid clashes (e.g. with the Linux `barrier()`).
 *
 * Cost (what the macros add inside the measured loop):
 *   +-----------------------------+----------------------+------------------+
 *   | macro                       | GCC/Clang/ICC/clang-cl| MSVC / fallback |
 *   +-----------------------------+----------------------+------------------+
 *   | bench_do_not_optimize       | 0 instructions       | 1 pointer store  |
 *   | bench_launder               | 0 instructions       | 1 pointer store  |
 *   | bench_escape                | 0 instructions       | 1 pointer store  |
 *   | bench_clobber_memory        | 0 instructions       | 0 (MSVC) / 1     |
 *   |                             |                      | indirect call    |
 *   +-----------------------------+----------------------+------------------+
 *   "0 instructions" means the macro itself emits no code. Making a value
 *   observable may still force a move or a spill of that one value, and the
 *   memory clobber of bench_escape/bench_clobber_memory forces the compiler to
 *   reload memory-resident values afterwards.
 *
 * Limitations (read these):
 *   - bench_do_not_optimize alone does NOT stop loop-invariant hoisting or
 *     constant folding of the *inputs*; use bench_launder for that.
 *   - Compiler barriers only. None of these is a CPU/hardware fence.
 *   - bench_do_not_optimize/launder on GCC/Clang use a general register
 *     constraint: floating-point values cost one register move, and
 *     `long double` or aggregates are rejected at compile time. For those,
 *     use bench_escape(&obj).
 *   - Fallback compilers (neither GNU-compatible nor MSVC): relies on volatile
 *     accesses and an indirect call through a volatile function pointer. It is
 *     best-effort and only affects memory whose address has escaped.
 *   - Define BENCH_UTILS_FORCE_FALLBACK to exercise the fallback path on any
 *     compiler (useful in CI).
 *
 * Always verify on your toolchain by inspecting the generated assembly
 * (gcc/clang -S, msvc /FA): the measured work must still be present.
 */

#pragma once

#if (defined(__GNUC__) || defined(__clang__)) && \
    !defined(BENCH_UTILS_FORCE_FALLBACK)

/*****************************************************************
 * GCC, Clang (including clang-cl), ICC and compatibles          *
 * Empty inline asm: emits no instruction, only constraints.     *
 *****************************************************************/

/* Register-only ("r"): never forces a spill, rejects non-scalars. */
#define BENCH_USE_(v)       __asm__ __volatile__("" : : "r"(v))
#define BENCH_LAUNDER_(v)   __asm__ __volatile__("" : "+r"(v))
#define BENCH_ESCAPE_(p)    __asm__ __volatile__("" : : "r"(p) : "memory")
#define BENCH_CLOBBER_()    __asm__ __volatile__("" : : : "memory")

/* The asm operands already require a modifiable lvalue for launder. */
#define BENCH_CHECK_MODIFIABLE_(v) ((void) 0)

#elif defined(_MSC_VER) && !defined(BENCH_UTILS_FORCE_FALLBACK)

/*****************************************************************
 * MSVC (x86, x64, ARM, ARM64): no inline asm on x64/ARM64.      *
 * Escape the address through a volatile object, then issue a    *
 * compiler-only barrier.                                        *
 *****************************************************************/

#include <intrin.h>
#pragma intrinsic(_ReadWriteBarrier)

static inline void
bench_clobber_memory_impl_(void)
{
    _ReadWriteBarrier();
}

static inline void
bench_escape_impl_(const void *p)
{
    static const void *volatile sink;
    sink = p;                   /* observable: p (and its pointee) must exist */
    _ReadWriteBarrier();        /* ...and be up to date before this point */
}

#define BENCH_USE_(v)       bench_escape_impl_(&(v))
#define BENCH_LAUNDER_(v)   bench_escape_impl_(&(v))
#define BENCH_ESCAPE_(p)    bench_escape_impl_(p)
#define BENCH_CLOBBER_()    bench_clobber_memory_impl_()

/* Unevaluated: rejects const lvalues, like the GNU "+r" operand does. */
#define BENCH_CHECK_MODIFIABLE_(v) ((void) sizeof((v) = (v)))

#else

/*****************************************************************
 * Portable fallback: standard C only, best-effort.              *
 *****************************************************************/

static inline void
bench_noop_(void)
{
}

static inline void
bench_clobber_memory_impl_(void)
{
    /* The compiler must assume an unknown function may touch escaped memory */
    static void (*volatile fn)(void) = bench_noop_;
    fn();
}

static inline void
bench_escape_impl_(const void *p)
{
    static const void *volatile sink;
    sink = p;                   /* volatile store: observable side effect */
    bench_clobber_memory_impl_();
}

#define BENCH_USE_(v)       bench_escape_impl_(&(v))
#define BENCH_LAUNDER_(v)   bench_escape_impl_(&(v))
#define BENCH_ESCAPE_(p)    bench_escape_impl_(p)
#define BENCH_CLOBBER_()    bench_clobber_memory_impl_()

#define BENCH_CHECK_MODIFIABLE_(v) ((void) sizeof((v) = (v)))

#endif

/*****************************************************************
 * Public API (identical on every compiler)                      *
 *****************************************************************/

#define bench_do_not_optimize(value)                                          \
    do {                                                                      \
        (void) sizeof(&(value));    /* must be an lvalue, on every compiler */\
        BENCH_USE_(value);                                                    \
    } while (0)

#define bench_launder(value)                                                  \
    do {                                                                      \
        (void) sizeof(&(value));                                              \
        BENCH_CHECK_MODIFIABLE_(value);                                       \
        BENCH_LAUNDER_(value);                                                \
    } while (0)

#define bench_escape(ptr)                                                     \
    do {                                                                      \
        const void *bench_escape_p_ = (ptr);    /* must convert to a pointer */\
        BENCH_ESCAPE_(bench_escape_p_);                                       \
    } while (0)

#define bench_clobber_memory()                                                \
    do {                                                                      \
        BENCH_CLOBBER_();                                                     \
    } while (0)
