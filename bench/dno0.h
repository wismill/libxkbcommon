/*
 * SPDX-License-Identifier: MIT
 *
 * Compiler escape hatches for benchmarks.
 *
 * do_not_optimize(value)
 *   Makes `value` look like it is used by something outside the compiler's
 *   knowledge, so the computation producing it cannot be dead-code
 *   eliminated. Generates no code on GCC/Clang/ICC/clang-cl; one stack
 *   store on MSVC; one volatile load-add-store on the generic fallback.
 *
 *   Caveats:
 *   - It does NOT prevent loop-invariant hoisting: a loop-invariant value
 *     may still be computed once outside the loop. Combine with bench_barrier()
 *     or make the inputs vary inside the loop when that matters.
 *   - It does NOT prevent compile-time constant folding of values that are
 *     known at compile time (irrelevant when consuming opaque library calls).
 *   - `value` must be a scalar or pointer; aggregates are not supported by
 *     the asm constraint. In the MSVC and fallback paths it must also be
 *     an lvalue (assign to a local first).
 *
 * bench_barrier()
 *   Compiler-level memory barrier: loads and stores cannot be reordered
 *   across it, and memory accesses cannot sink out of loops past it.
 *   Emits no code and is not a hardware/CPU barrier.
 *
 * Usage (benchmark sink):
 *
 *     for (...) {
 *         const int r = work(...);
 *         do_not_optimize(r);
 *     }
 *
 * Verify on your setup by checking the generated assembly
 * (gcc -S / clang -S / msvc /FA): the call must expand to no instructions
 * (or a single store for MSVC).
 */

#pragma once

#if defined(__GNUC__) || defined(__clang__)

/*************************************************************
 * GCC, Clang (incl. clang-cl), ICC: inline-asm escape hatch *
 ************************************************************/

/*
 * Empty asm template with `value` as an input operand: the value must be
 * materialized (register or memory), but no instruction is emitted.
 * The "memory" clobber additionally makes it a compiler barrier.
 */
#define do_not_optimize(value) \
    __asm__ __volatile__("" : : "g"(value))

#define bench_barrier() \
    __asm__ __volatile__("" : : : "memory")

#elif defined(_MSC_VER)

/*********************************************
 * MSVC, x86 and x64 (x64 has no inline asm) *
 ********************************************/

#include <intrin.h>

/*
 * An access through a volatile char pointer is an observable side effect
 * (C11 6.7.3p6), so it cannot be optimized away; taking the address forces
 * the value to be materialized in memory at this point. Cost: one store
 * (the byte load is store-forwarded). `value` must be an lvalue.
 * This is the same idea as Google Benchmark’s MSVC path.
 */
#define do_not_optimize(value) do {         \
    (void)*(char const volatile *) &(value);\
    _ReadWriteBarrier();                    \
} while (0)

#define bench_barrier() _ReadWriteBarrier()

#else

/***********************************************
 * Portable fallback: standard C, but not free *
 **********************************************/

/*
 * Stores to a volatile object are observable side effects, so no conforming
 * compiler can optimize this away. Cost: one load + add + store per call,
 * i.e. exactly the classic `volatile uintptr_t acc` sink.
 * Pass scalars as unsigned int; cast pointers with (uintptr_t).
 */
#include <stdint.h>

static volatile uintptr_t dno_sink_;

static inline void
do_not_optimize(uintptr_t value)
{
    dno_sink_ += value;
}

static inline void
bench_barrier(void)
{
    do_not_optimize(0);
}

#endif
