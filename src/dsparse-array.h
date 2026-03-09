/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

/**
 * dsparse_array.h — C11 generic sparse array
 *
 * A sparse array that avoids heap allocation entirely when holding 0 or 1
 * items.  The layout is driven by a bitmask whose set-bit count equals the
 * number of live elements:
 *
 * • count == 0: empty
 * • count == 1: inline storage
 * • count >= 2: heap storage
 *
 * Usage: declare a typed sparse-array struct with the macro, then call the
 * matching _init / _free / _merge helpers.
 *
 * ```c
 * // 1. Declare your type: sparse array of up to 16 ints
 * typedef dsparse_array(int, uint16_t, uint8_t, true, one, multi)
 *     int_sparse_t;
 *
 * // 2. Instantiate helpers for that type
 * dsparse_array_impl(int_sparse_t, int, uint16_t, uint8_t, true, one, multi)
 *
 * // 3. Use
 * int_sparse_t sa = int_sparse_t_new();
 * int_sparse_t_insert(&sa, 3, 42);
 * …
 * int_sparse_t_free(&sa);
 * ```
 */

#pragma once

#include "config.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define _DSPARSE_ARRAY_MIN(a, b) ((a) < (b) ? (a) : (b))

#if defined(__GNUC__) || defined(__clang__)
#  define _DSPARSE_ARRAY_UNUSED __attribute__((unused))
#else
#  define _DSPARSE_ARRAY_UNUSED
#endif

/* -------------------------------------------------------------------------
 * Part 1 – struct declaration macro
 * ------------------------------------------------------------------------- */

/* Helpers for optional count field */
#define dsparse_array_if_0(...)
#define dsparse_array_if_false(...)
#define dsparse_array_if_1(...)         __VA_ARGS__
#define dsparse_array_if_true(...)      __VA_ARGS__
#define dsparse_array_if_not_0(...)     __VA_ARGS__
#define dsparse_array_if_not_false(...) __VA_ARGS__
#define dsparse_array_if_not_1(...)
#define dsparse_array_if_not_true(...)

/**
 * Expands to an *anonymous struct* that can be used with typedef:
 *
 *   typedef dsparse_array(float, uint32_t, uint8_t, true, one, multi)
 *       float_sparse_t;
 */
#define dsparse_array(ITEM_TYPE, MASK_TYPE, COUNT_TYPE,       \
                      HAS_COUNT_FIELD, ONE_FIELD, MULTI_FIELD)\
    struct {                                                  \
        /** Indices bitmask: bit i set ⇔ logical index i */   \
        MASK_TYPE mask;                                       \
        dsparse_array_if_##HAS_COUNT_FIELD(                   \
            /** Exact item count */                           \
            COUNT_TYPE count                                  \
        );                                                    \
        /* Inline item count or allocated slot count */       \
        union {                                               \
            /** Number of *inline* items if count ≤ 1 */      \
            COUNT_TYPE num_inline;                            \
            /** Number of *allocated* slots if count > 1      \
             *  Can be greater than the real item count */    \
            COUNT_TYPE alloc;                                 \
        };                                                    \
        union {                                               \
            /** count ≤ 1: inline storage */                  \
            ITEM_TYPE  ONE_FIELD;                             \
            /** count > 1: heap storage */                    \
            ITEM_TYPE *MULTI_FIELD;                           \
        };                                                    \
    }

#define dsparse_array_new() { .num_inline = 0, .mask = 0 }

/** Initialise an already-declared sparse array to the empty state. */
#define dsparse_array_init(HAS_COUNT_FIELD, MULTI_FIELD, sa) do { \
    (sa).mask = 0;                                                \
    (sa).num_inline = 0;                                          \
    dsparse_array_if_##HAS_COUNT_FIELD((sa).count = 0);           \
    (sa).MULTI_FIELD = NULL; /* zero the union */                 \
} while (0)

/** Release heap memory (if any) and reset to empty. */
#define dsparse_array_free(HAS_COUNT_FIELD, MULTI_FIELD, sa) do { \
    if ((sa).num_inline > 1)                                      \
        free((void *)(sa).MULTI_FIELD);                           \
    dsparse_array_init(HAS_COUNT_FIELD, MULTI_FIELD, sa);         \
} while (0)

/** Return whether the container is empty or not */
#define dsparse_array_empty(sa) (!(sa).mask)

/** Get the item count */
#define dsparse_array_count(HAS_COUNT_FIELD, COUNT_TYPE, sa) ( \
    dsparse_array_if_##HAS_COUNT_FIELD((sa).count)             \
    dsparse_array_if_not_##HAS_COUNT_FIELD(                    \
        (COUNT_TYPE)_SPARSE_POPCOUNT((sa).mask)                \
    )                                                          \
)                                                              \

/** Get the mask */
#define dsparse_array_mask(sa) (sa.mask)

/* -------------------------------------------------------------------------
 * Part 2 – popcount helpers, one per natural mask width
 *
 * Each function uses the tightest available intrinsic for its width so the
 * compiler can emit a single POPCNT instruction (or the fastest equivalent
 * software fallback) without unnecessary zero-extension to 64 bits.
 *
 * _Generic then selects the right overload based on the *exact type* of the
 * mask expression — no implicit widening, no cast noise at the call site.
 * ------------------------------------------------------------------------- */

static inline _DSPARSE_ARRAY_UNUSED unsigned int
_sparse_popcount8(uint8_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount((unsigned int)x);
#else
    x = (uint8_t)(x - ((x >> 1) & UINT8_C(0x55)));
    x = (uint8_t)((x & UINT8_C(0x33)) + ((x >> 2) & UINT8_C(0x33)));
    x = (uint8_t)((x + (x >> 4)) & UINT8_C(0x0f));
    return (unsigned int)x;
#endif
}

static inline _DSPARSE_ARRAY_UNUSED unsigned int
_sparse_popcount16(uint16_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount((unsigned int)x);
#else
    x = (uint16_t)(x - ((x >> 1) & UINT16_C(0x5555)));
    x = (uint16_t)((x & UINT16_C(0x3333)) + ((x >> 2) & UINT16_C(0x3333)));
    x = (uint16_t)((x + (x >> 4)) & UINT16_C(0x0f0f));
    return (unsigned int)((uint16_t)(x * UINT16_C(0x0101)) >> 8);
#endif
}

static inline _DSPARSE_ARRAY_UNUSED unsigned int
_sparse_popcount32(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(x);
#else
    x = x - ((x >> 1) & UINT32_C(0x55555555));
    x = (x & UINT32_C(0x33333333)) + ((x >> 2) & UINT32_C(0x33333333));
    x = (x + (x >> 4)) & UINT32_C(0x0f0f0f0f);
    return (unsigned int)((x * UINT32_C(0x01010101)) >> 24);
#endif
}

static inline _DSPARSE_ARRAY_UNUSED unsigned int
_sparse_popcount64(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return (unsigned int)__builtin_popcountll(x);
#else
    x = x - ((x >> 1) & UINT64_C(0x5555555555555555));
    x = (x & UINT64_C(0x3333333333333333)) + ((x >> 2) & UINT64_C(0x3333333333333333));
    x = (x + (x >> 4)) & UINT64_C(0x0f0f0f0f0f0f0f0f);
    return (unsigned int)((x * UINT64_C(0x0101010101010101)) >> 56);
#endif
}

/**
 * _SPARSE_POPCOUNT(mask)
 *
 * Dispatches to the width-appropriate helper via C11 _Generic.
 *
 * Supported mask types: uint8_t, uint16_t, uint32_t, uint64_t.
 * Any other type triggers a compile-time error ("no matching generic").
 */
#define _SPARSE_POPCOUNT(mask)        \
    _Generic((mask),                  \
        uint8_t:  _sparse_popcount8,  \
        uint16_t: _sparse_popcount16, \
        uint32_t: _sparse_popcount32, \
        uint64_t: _sparse_popcount64  \
    )(mask)


/* -------------------------------------------------------------------------
 * Part 3 – next power of 2 helpers
 * ------------------------------------------------------------------------- */

#if defined(__GNUC__) || defined(__clang__)
/* Use builtins */

static inline _DSPARSE_ARRAY_UNUSED unsigned
next_pow2_unsigned(unsigned x)
{
    if (x <= 1) return 1u;
    return 1u << (sizeof(unsigned)*CHAR_BIT - __builtin_clz(x - 1));
}

static inline _DSPARSE_ARRAY_UNUSED unsigned long
next_pow2_unsigned_long(unsigned long x)
{
    if (x <= 1) return 1ul;
    return 1ul << (sizeof(unsigned long)*CHAR_BIT - __builtin_clzl(x - 1));
}

static inline _DSPARSE_ARRAY_UNUSED unsigned long long
next_pow2_unsigned_long_long(unsigned long long x)
{
    if (x <= 1) return 1ull;
    return 1ull << (sizeof(unsigned long long)*CHAR_BIT - __builtin_clzll(x - 1));
}

#else
/* portable fallback */

#define DEFINE_NEXT_POW2(T, NAME)                         \
static inline T _DSPARSE_ARRAY_UNUSED                     \
next_pow2_##NAME(T v)                                     \
{                                                         \
    if (v <= 1u) return 1;                                \
    v--;                                                  \
    for (unsigned s = 1; s < sizeof(T)*CHAR_BIT; s <<= 1) \
        v |= v >> s;                                      \
    return v + 1u;                                        \
}

DEFINE_NEXT_POW2(unsigned, unsigned)
DEFINE_NEXT_POW2(unsigned long, unsigned_long)
DEFINE_NEXT_POW2(unsigned long long, unsigned_long_long)

#endif

#define next_pow2(x) _Generic((x), \
    unsigned char:      next_pow2_unsigned, \
    unsigned short:     next_pow2_unsigned, \
    unsigned int:       next_pow2_unsigned, \
    unsigned long:      next_pow2_unsigned_long, \
    unsigned long long: next_pow2_unsigned_long_long \
)(x)

/* -------------------------------------------------------------------------
 * Part 4 – function-body macros (implement the API for a concrete typedef)
 *
 * bit and rank are both typed as COUNT_TYPE:
 *   - bit  is a position in the mask, bounded by sizeof(MASK_TYPE)*8,
 *                which is always <= the maximum value of COUNT_TYPE.
 *   - rank       is the number of set bits below a given position, so it is
 *                also bounded by count, making COUNT_TYPE a natural fit.
 * ------------------------------------------------------------------------- */

/**
 * Emits static-inline functions named <SA_TYPE>_init, <SA_TYPE>_free,
 * <SA_TYPE>_mask, <SA_TYPE>_count, <SA_TYPE>_insert, <SA_TYPE>_get,
 * <SA_TYPE>_merge for the struct type SA_TYPE.
 *
 * All functions return true on success, false on failure.
 */
#define dsparse_array_impl(SA_TYPE, ITEM_TYPE, MASK_TYPE, COUNT_TYPE,          \
                           HAS_COUNT_FIELD, ONE_FIELD, MULTI_FIELD)            \
                                                                               \
static_assert((MASK_TYPE)(-1) > (MASK_TYPE)0,                                  \
              "MASK_TYPE must be an unsigned type");                           \
static_assert((COUNT_TYPE)(-1) > (COUNT_TYPE)0,                                \
              "COUNT_TYPE must be an unsigned type");                          \
static_assert(sizeof(MASK_TYPE) * CHAR_BIT - 1 <= ((COUNT_TYPE)(~(COUNT_TYPE)0)),\
              #SA_TYPE ": COUNT_TYPE too narrow to index all bits of MASK_TYPE");\
dsparse_array_if_##HAS_COUNT_FIELD(                                            \
    static_assert(2 * sizeof(COUNT_TYPE) + sizeof(MASK_TYPE) <=                \
                  sizeof(union {ITEM_TYPE a; ITEM_TYPE *b;}),                  \
                  #SA_TYPE ": count field requires extra padding")             \
);                                                                             \
                                                                               \
/** Initialise an already-declared sparse array to the empty state. */         \
static inline _DSPARSE_ARRAY_UNUSED void                                       \
SA_TYPE##_init(SA_TYPE *sa)                                                    \
{                                                                              \
    dsparse_array_init(HAS_COUNT_FIELD, MULTI_FIELD, *sa);                     \
}                                                                              \
                                                                               \
/** Release heap memory (if any) and reset to empty. */                        \
static inline _DSPARSE_ARRAY_UNUSED void                                       \
SA_TYPE##_free(SA_TYPE *sa)                                                    \
{                                                                              \
    dsparse_array_free(HAS_COUNT_FIELD, MULTI_FIELD, *sa);                     \
}                                                                              \
                                                                               \
/** Return whether the container is empty or not */                            \
static inline _DSPARSE_ARRAY_UNUSED bool                                       \
SA_TYPE##_empty(const SA_TYPE *sa)                                             \
{                                                                              \
    return !sa->mask;                                                          \
}                                                                              \
                                                                               \
/** Get the item count */                                                      \
static inline _DSPARSE_ARRAY_UNUSED COUNT_TYPE                                 \
SA_TYPE##_count(const SA_TYPE *sa)                                             \
{                                                                              \
    return dsparse_array_count(HAS_COUNT_FIELD, COUNT_TYPE, *sa);              \
}                                                                              \
                                                                               \
/** Get the mask */                                                            \
static inline _DSPARSE_ARRAY_UNUSED MASK_TYPE                                  \
SA_TYPE##_mask(const SA_TYPE *sa)                                              \
{                                                                              \
    return sa->mask;                                                           \
}                                                                              \
                                                                               \
/**                                                                            \
 * Insert value at logical index `bit` (0-based bit position in mask).         \
 * If the bit is already set the existing value is overwritten.                \
 * Returns true on success, false on OOM or invalid index.                     \
 * On OOM, the array is left in its previous valid state; the caller           \
 * is responsible for calling _free() if it cannot recover.                    \
 */                                                                            \
static inline _DSPARSE_ARRAY_UNUSED bool                                       \
SA_TYPE##_insert(SA_TYPE *sa, COUNT_TYPE bit, ITEM_TYPE value)                 \
{                                                                              \
    if (bit >= (COUNT_TYPE)(sizeof(MASK_TYPE) * CHAR_BIT))                     \
        return false;                                                          \
                                                                               \
    const MASK_TYPE mask = (MASK_TYPE)((MASK_TYPE)1 << bit);                   \
                                                                               \
    if (sa->mask & mask) {                                                     \
        /* Overwrite existing slot */                                          \
        if (sa->num_inline == 1) {                                             \
            sa->ONE_FIELD = value; /* rank must be 0 */                        \
        } else {                                                               \
            const MASK_TYPE low = (MASK_TYPE)(sa->mask & (MASK_TYPE)(mask - 1));\
            /* index = number of set bits strictly below bit */                \
            const COUNT_TYPE index = (COUNT_TYPE)_SPARSE_POPCOUNT(low);        \
            sa->MULTI_FIELD[index] = value;                                    \
        }                                                                      \
        return true;                                                           \
    }                                                                          \
                                                                               \
    /* Insert new element */                                                   \
    if (sa->num_inline == 0) {                                                 \
        /* 0 → 1 : use inline storage, no alloc */                             \
        sa->ONE_FIELD = value;                                                 \
        sa->num_inline = 1;                                                    \
        dsparse_array_if_##HAS_COUNT_FIELD(sa->count = 1);                     \
    } else if (sa->num_inline == 1) {                                          \
        /* 1 → 2 : promote inline element to heap */                           \
        ITEM_TYPE * const arr = malloc(2 * sizeof(ITEM_TYPE));                 \
        if (!arr) return false;                                                \
        const ITEM_TYPE old = sa->ONE_FIELD;                                   \
        if (mask < sa->mask) { arr[0] = value; arr[1] = old;   }               \
        else                 { arr[0] = old;   arr[1] = value; }               \
        sa->MULTI_FIELD = arr;                                                 \
        sa->alloc = 2;                                                         \
        dsparse_array_if_##HAS_COUNT_FIELD(sa->count = 2);                     \
    } else {                                                                   \
        /* Allocated items: grow heap and shift if relevant */                 \
        const COUNT_TYPE count = SA_TYPE##_count(sa);                          \
        ITEM_TYPE *arr;                                                        \
        COUNT_TYPE alloc;                                                      \
        if (count + 1u > sa->alloc) {                                          \
            const COUNT_TYPE pow2_alloc = next_pow2((COUNT_TYPE)(count + 1u)); \
            alloc = _DSPARSE_ARRAY_MIN(pow2_alloc, (COUNT_TYPE)(count + 8));   \
            arr = realloc(sa->MULTI_FIELD, (size_t)alloc * sizeof(ITEM_TYPE)); \
            if (!arr) return false;                                            \
        } else {                                                               \
            alloc = sa->alloc;                                                 \
            arr = sa->MULTI_FIELD;                                             \
        }                                                                      \
        const MASK_TYPE  low   = (MASK_TYPE)(sa->mask & (MASK_TYPE)(mask - 1));\
        const COUNT_TYPE index = (COUNT_TYPE)_SPARSE_POPCOUNT(low);            \
        if (index < count) {                                                   \
            /* insert between 2 previous items: move items from greater bits */\
            assert(index + 1 < alloc);                                         \
            memmove(arr + index + 1, arr + index,                              \
                    (size_t)(count - index) * sizeof(ITEM_TYPE));              \
        }                                                                      \
        arr[index] = value;                                                    \
        sa->MULTI_FIELD = arr;                                                 \
        sa->alloc = alloc;                                                     \
        dsparse_array_if_##HAS_COUNT_FIELD(sa->count++);                       \
    }                                                                          \
                                                                               \
    sa->mask = (sa->mask | mask);                                              \
    return true;                                                               \
}                                                                              \
                                                                               \
/**                                                                            \
 * Retrieve the value at logical index `bit`.                                  \
 * Returns true and writes *out_value if the bit is set, false otherwise.      \
 */                                                                            \
static inline _DSPARSE_ARRAY_UNUSED bool                                       \
SA_TYPE##_get(const SA_TYPE *sa, COUNT_TYPE bit, ITEM_TYPE *out_value)         \
{                                                                              \
    if (bit >= (COUNT_TYPE)(sizeof(MASK_TYPE) * CHAR_BIT))                     \
        return false;                                                          \
    const MASK_TYPE mask = (MASK_TYPE)((MASK_TYPE)1 << bit);                   \
    if (!(sa->mask & mask)) return false;                                      \
    if (out_value) {                                                           \
        if (sa->num_inline == 1) {                                             \
            *out_value = sa->ONE_FIELD;                                        \
        } else {                                                               \
            const MASK_TYPE low = (MASK_TYPE)(sa->mask & (MASK_TYPE)(mask - 1));\
            const COUNT_TYPE index = (COUNT_TYPE)_SPARSE_POPCOUNT(low);        \
            *out_value = sa->MULTI_FIELD[index];                               \
        }                                                                      \
    }                                                                          \
    return true;                                                               \
}                                                                              \
                                                                               \
/**                                                                            \
 * Merge src into dst.                                                         \
 * For bits present in both, src’s value wins (last-write semantics).          \
 * dst is modified in place; src is left untouched.                            \
 * Returns true on success, false on OOM (dst may be partially updated on failure).\
 */                                                                            \
static inline _DSPARSE_ARRAY_UNUSED bool                                       \
SA_TYPE##_merge(SA_TYPE * restrict dst, const SA_TYPE * restrict src)          \
{                                                                              \
    const MASK_TYPE merged_mask = dst->mask | src->mask;                       \
    if (!merged_mask)                                                          \
        return true;                                                           \
    const COUNT_TYPE merged_count = (COUNT_TYPE)_SPARSE_POPCOUNT(merged_mask); \
    if (merged_count > 1 && merged_count > SA_TYPE##_count(dst)) {             \
        /* Avoid reallocating repeatedly */                                    \
        ITEM_TYPE *values;                                                     \
        if (dst->num_inline <= 1) {                                            \
            values = malloc(merged_count * sizeof(ITEM_TYPE));                 \
            if (!values) return false;                                         \
            *values = dst->ONE_FIELD;                                          \
        } else {                                                               \
            values = realloc(dst->MULTI_FIELD,                                 \
                             (size_t)merged_count * sizeof(ITEM_TYPE));        \
            if (!values) return false;                                         \
        }                                                                      \
        dst->MULTI_FIELD = values;                                             \
        dst->alloc = merged_count;                                             \
    }                                                                          \
    MASK_TYPE remaining = src->mask;                                           \
    const ITEM_TYPE *values = (src->num_inline <= 1) ? &src->ONE_FIELD         \
                                                     : src->MULTI_FIELD;       \
    while (remaining) {                                                        \
        /* isolate lowest set bit */                                           \
        const MASK_TYPE lsb =                                                  \
            (MASK_TYPE)(remaining & (MASK_TYPE)(~remaining + (MASK_TYPE)1));   \
        /* get its index */                                                    \
        const COUNT_TYPE bit =                                                 \
            (COUNT_TYPE)_SPARSE_POPCOUNT((MASK_TYPE)(lsb - (MASK_TYPE)1));     \
        /* pop current value */                                                \
        const ITEM_TYPE val = *(values++);                                     \
        if (!SA_TYPE##_insert(dst, bit, val)) return false;                    \
        remaining = (remaining & ~lsb);                                        \
    }                                                                          \
    return true;                                                               \
}

/* -------------------------------------------------------------------------
 * Part 5 – Helpers
 * ------------------------------------------------------------------------- */

/** Declare + implement in one shot */
#define dsparse_array_define(SA_TYPE, ITEM_TYPE, MASK_TYPE, COUNT_TYPE,      \
                             HAS_COUNT_FIELD, ONE_FIELD, MULTI_FIELD)        \
    typedef dsparse_array(ITEM_TYPE, MASK_TYPE, COUNT_TYPE,                  \
                          HAS_COUNT_FIELD, ONE_FIELD, MULTI_FIELD) (SA_TYPE);\
    dsparse_array_impl(SA_TYPE, ITEM_TYPE, MASK_TYPE, COUNT_TYPE,            \
                       HAS_COUNT_FIELD, ONE_FIELD, MULTI_FIELD)

/** Traversal */
#define __dsparse_array_foreach(SA_TYPE, ITEM_TYPE, MASK_TYPE, COUNT_TYPE,     \
                                ONE_FIELD, MULTI_FIELD, tmp, sa, val, ...)     \
    if ((sa).mask)                                                             \
    for (                                                                      \
        struct {                                                               \
            COUNT_TYPE bit;                                                    \
            MASK_TYPE remaining;                                               \
            MASK_TYPE lsb;                                                     \
            const ITEM_TYPE *values;                                           \
        } tmp = {                                                              \
            .remaining = (sa).mask,                                            \
            .values = ((sa).num_inline <= 1) ? &(sa).ONE_FIELD                 \
                                             : (sa).MULTI_FIELD                \
        };                                                                     \
        (tmp).remaining && (                                                   \
            val = (tmp).values,                                                \
            /* isolate lowest set bit */                                       \
            (tmp).lsb = (MASK_TYPE)(                                           \
                (tmp).remaining & (MASK_TYPE)(~(tmp).remaining + (MASK_TYPE)1) \
            ),                                                                 \
            __VA_OPT__(                                                        \
                /* get its index */                                            \
                ((tmp).bit = (COUNT_TYPE)                                      \
                    _SPARSE_POPCOUNT((MASK_TYPE)((tmp).lsb - (MASK_TYPE)1)),   \
                (__VA_ARGS__) = (tmp).bit),                                    \
            )                                                                  \
            true                                                               \
        );                                                                     \
        (tmp).remaining = ((tmp).remaining & ~(tmp).lsb),                      \
        (tmp).values++                                                         \
    )

#define dsparse_array_foreach(SA_TYPE, ITEM_TYPE, MASK_TYPE, COUNT_TYPE,       \
                              ONE_FIELD, MULTI_FIELD, sa, val, ...)            \
    __dsparse_array_foreach(SA_TYPE, ITEM_TYPE, MASK_TYPE, COUNT_TYPE,         \
                            ONE_FIELD, MULTI_FIELD, __##SA_TYPE##_foreach_tmp, \
                            sa, val, __VA_ARGS__)
