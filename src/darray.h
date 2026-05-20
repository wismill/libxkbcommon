/*
 * Copyright (C) 2011 Joseph Adams <joeyadams3.14159@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

/* Originally taken from: https://ccodearchive.net/info/darray.html
 * But modified for libxkbcommon. */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "utils.h"

/* FIXME: remove GCC hack */
#if defined(__GNUC__) && !defined(__clang__)
#  define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#  define REQUIRE_GCC_HACK (GCC_VERSION >= 1601)
#else
#  define GCC_VERSION 0
#  define REQUIRE_GCC_HACK 0
#endif

typedef unsigned int darray_size_t;
#define darray_max_alloc(item_size) (UINT_MAX / (item_size))

#define darray(type) struct {         \
    /** Count of allocated items */   \
    darray_size_t alloc;              \
    /** Current size */               \
    darray_size_t size;               \
    /** Array of items */             \
    type *item ATTR_COUNTED_BY(size); \
}

enum {
    DARRAY_SIZE_T_WIDTH = sizeof(darray_size_t) * CHAR_BIT,
    DARRAY_SIZE_MAX = UINT_MAX
};

#define darray_new() { 0, 0, NULL }

#define darray_init(arr) do { \
    (arr).item = NULL; (arr).size = 0; (arr).alloc = 0; \
} while (0)

#define darray_free(arr) do { \
    free((arr).item); \
    darray_init(arr); \
} while (0)

#define darray_steal(arr, to, ...) do { \
    *(to) = (arr).item; \
    __VA_OPT__(if (__VA_ARGS__) \
        *(__VA_ARGS__) = (arr).size;) \
    darray_init(arr); \
} while (0)

/*
 * Typedefs for darrays of common types.  These are useful
 * when you want to pass a pointer to an darray(T) around.
 *
 * The following will produce an incompatible pointer warning:
 *
 *     void foo(darray(int) *arr);
 *     darray(int) arr = darray_new();
 *     foo(&arr);
 *
 * The workaround:
 *
 *     void foo(darray_int *arr);
 *     darray_int arr = darray_new();
 *     foo(&arr);
 */

typedef darray (char)           darray_char;
typedef darray (signed char)    darray_schar;
typedef darray (unsigned char)  darray_uchar;

typedef darray (char *)         darray_string;

typedef darray (short)          darray_short;
typedef darray (int)            darray_int;
typedef darray (long)           darray_long;

typedef darray (unsigned short) darray_ushort;
typedef darray (unsigned int)   darray_uint;
typedef darray (unsigned long)  darray_ulong;

/*** Access ***/

#define darray_item(arr, i)     ((arr).item[i])
#define darray_items(arr)       ((arr).item)
#define darray_size(arr)        ((arr).size)
#define darray_empty(arr)       ((arr).size == 0)

/*** Size management ***/

#define darray_die(fmt, ...) do { \
    fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

ATTRIBS(NODISCARD) bool
darray_heap_grow(void ** restrict data, size_t item_size,
                 darray_size_t * restrict capacity,
                 darray_size_t need);

ATTRIBS(NODISCARD) bool
darray_heap_shrink(void ** restrict data, size_t item_size,
                   darray_size_t * restrict capacity,
                   darray_size_t size);

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_resize_(void ** restrict data, size_t item_size,
               darray_size_t * restrict capacity, darray_size_t * restrict size,
               darray_size_t need)
{
    if (unlikely(!darray_heap_grow(data, item_size, capacity, need))) {
        darray_die("ERROR: failed to allocate in %s()\n", __func__);
        // return false;
    }

    *size = need;
    return true;
}

#define darray_resize(arr, new_size) \
    darray_resize_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                   &(arr).size, (new_size))

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_resize0_(void ** restrict data, size_t item_size,
                darray_size_t * restrict capacity, darray_size_t * restrict size,
                darray_size_t need)
{
    const darray_size_t old_size = *size;

    if (unlikely(!darray_resize_(data, item_size, capacity, size, need)))
        return false;

    if (old_size < need) {
        memset((char*)*data + old_size * item_size, 0,
               (need - old_size) * item_size);
    }

    return true;
}

#define darray_resize0(arr, new_size)  \
    darray_resize0_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                    &(arr).size, (new_size))

#define darray_shrink(arr) \
    darray_heap_shrink((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                       (arr).size)

/*** Insertion (single item) ***/

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_append_(void ** restrict data, size_t item_size,
               darray_size_t * restrict capacity, darray_size_t * restrict size,
               const void * restrict item)
{
    /* Item cannot belong the array */
    assert(!*data || item < *data ||
           (uintptr_t)*data + (uintptr_t)(*capacity * item_size) <= (uintptr_t)item);

    if (unlikely(!darray_resize_(data, item_size, capacity, size, *size + 1)))
        return false;

    memcpy((char *)*data + (*size - 1) * item_size, item, item_size);

    return true;
}

#define darray_append(arr, i) \
    darray_append_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                   &(arr).size, &(i))

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_insert_(void ** restrict data, size_t item_size,
               darray_size_t * restrict capacity, darray_size_t * restrict size,
               darray_size_t index, const void * restrict item)
{
    /* Item cannot belong to the array */
    assert(!*data || item < *data ||
           (uintptr_t)*data + (uintptr_t)(*capacity * item_size) <= (uintptr_t)item);

    if (unlikely(!darray_resize_(data, item_size, capacity, size, *size + 1)))
        return false;

    memmove(
        (char*)*data + (index + 1) * item_size,
        (char*)*data + index * item_size,
        (*size - index - 1) * item_size
    );

    memcpy((char *)*data + index * item_size, item, item_size);

    return true;
}

#define darray_insert(arr, index, i) \
    darray_insert_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                   &(arr).size, (index), &(i))

/*** Insertion (multiple items) ***/

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_append_items_(void ** restrict data, size_t item_size,
                     darray_size_t * restrict capacity,
                     darray_size_t * restrict size,
                     const void * restrict items, darray_size_t count)
{
    /* Items cannot belong to the array */
    assert(!*data || items < *data ||
           (uintptr_t)*data + (uintptr_t)(*capacity * item_size) <= (uintptr_t)items);

    if (unlikely(!darray_resize_(data, item_size, capacity, size, *size + count)))
        return false;

    memcpy((char *)*data + (*size - count) * item_size, items, count * item_size);

    return true;
}

#define darray_append_items(arr, is, count) \
    darray_append_items_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                         &(arr).size, (is), (count))

#define darray_concat(dest, source) \
    darray_append_items((dest), (source).item, (source).size)

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_concat_self_(void ** restrict data, size_t item_size,
                    darray_size_t * restrict capacity,
                    darray_size_t * restrict size,
                    darray_size_t index, darray_size_t count)
{
    if (unlikely(index + count > *size ||
                 !darray_resize_(data, item_size, capacity, size, *size + count)))
        return false;

    memcpy((char *)*data + (*size - count) * item_size,
           (char *)*data + index * item_size, count * item_size);

    return true;
}

#define darray_concat_self(arr, index, count) \
    darray_concat_self_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                         &(arr).size, (index), (count))

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_from_items_(void ** restrict data, size_t item_size,
                   darray_size_t * restrict capacity,
                   darray_size_t * restrict size,
                   const void * restrict items, darray_size_t count)
{
    if (unlikely(!count))
        return true;

    if (unlikely((*data || !darray_resize_(data, item_size, capacity, size, count))))
        return false;

    memcpy((char *)*data, items, count * item_size);

    return true;
}

#define darray_from_items(arr, is, count) \
    darray_from_items_((void **)&(arr).item, sizeof(*(arr).item), &(arr).alloc, \
                       &(arr).size, (is), (count))

#define darray_copy(dest, source) \
    darray_from_items((dest), (source).item, (source).size)

/*** Removal ***/

#define darray_remove_last(arr) do { \
    if ((arr).size) \
        --(arr).size; \
} while (0)

/* Warning, slow: Requires copying all elements after removed item. */
#define darray_remove(arr, i) do { \
    const darray_size_t __index = (i); \
    if (__index < (arr).size) { \
        if (__index != (arr).size - 1) { \
            memmove( \
                &(arr).item[__index], \
                &(arr).item[__index + 1], \
                ((arr).size - 1 - __index) * sizeof(*(arr).item) \
            ); \
        } \
        (arr).size--; \
    } \
} while (0)

/*** String buffer ***/

/* Same as `darray_append_items` and do count the final '\0' in the size */
#define darray_append_string0(arr, str) \
    darray_append_items((arr), (str), (darray_size_t)strlen(str) + 1)

/* Same as `darray_append_items` but do not count the final '\0' in the size */
ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_append_string_(darray_char * restrict arr, const char * restrict str, size_t len)
{
    if (unlikely(len > DARRAY_SIZE_MAX ||
                 !darray_append_items(*arr, str, (darray_size_t)len))) {
        return false;
    }

    arr->size--;
    return true;
}

/* Same as `darray_append_string` but do not count the final '\0' in the size */
#define darray_append_string(arr, str) \
    darray_append_string_(&(arr), (str), strlen(str) + 1)

#define darray_append_lit(arr, stringLiteral) \
    darray_append_string_(&(arr), \
                          (const char *){(stringLiteral)}, \
                          sizeof(stringLiteral))

ATTRIBS(NODISCARD, MAYBE_UNUSED) static inline bool
darray_appends_nullterminate(darray_char * restrict arr,
                             const char * restrict items,
                             darray_size_t count)
{
    /* Items cannot belong to the array */
    assert(!arr->alloc || items < arr->item ||
           /* casting required to bypass [[counted_by()]] */
           (uintptr_t)arr->item + arr->alloc <= (uintptr_t)items);

    const darray_size_t old_size = arr->size;
    const darray_size_t new_size = old_size + count + 1;

    if (unlikely(new_size <= old_size ||
                 !darray_resize(*arr, new_size))) {
        return false;
    }

    if (count) {
        memcpy(arr->item + old_size, items, count * sizeof(*arr->item));
    }

    arr->item[arr->size - 1] = '\0';
    arr->size--; /* Must come after, due to [[counted_by(size)]] */

    return true;
}

ATTRIBS(NODISCARD, MAYBE_UNUSED) static inline bool
darray_prepends_nullterminate(darray_char * restrict arr, const char * restrict items,
                              darray_size_t count)
{
    /* Items cannot belong to the array */
    assert(!arr->alloc || items < arr->item ||
           /* casting required to bypass [[counted_by()]] */
           (uintptr_t)arr->item + arr->alloc <= (uintptr_t)items);

    const darray_size_t old_size = arr->size;
    const darray_size_t new_size = old_size + count + 1;

    if (unlikely(new_size <= old_size ||
                 !darray_resize(*arr, new_size))) {
        return false;
    }


    if (count) {
        if (old_size) {
            memmove(arr->item + count, arr->item, old_size * sizeof(*arr->item));
        }
        memcpy(arr->item, items, count * sizeof(*arr->item));
    }

    arr->item[arr->size - 1] = '\0';
    arr->size--; /* Must come after, due to [[counted_by(size)]] */

    return true;
}

/*** Traversal ***/

#define darray_foreach(i, arr) \
    if ((arr).item) \
    for ((i) = &(arr).item[0]; (i) < &(arr).item[(arr).size]; (i)++)

#if REQUIRE_GCC_HACK

#define darray_foreach_from(i, arr, from) \
    if ((from) < (arr).size && (arr).item) \
    for ((i) = (void*)((char*)(arr).item + (from) * sizeof(*(arr).item)); (i) < &(arr).item[(arr).size]; (i)++)

#else

#define darray_foreach_from(i, arr, from) \
    if ((from) < (arr).size && (arr).item) \
    for ((i) = &(arr).item[from]; (i) < &(arr).item[(arr).size]; (i)++)

#endif

/* Iterate on index and value at the same time, like Python's enumerate. */
#define darray_enumerate(idx, val, arr) \
    if ((arr).item) \
    for ((idx) = 0, (val) = (arr).item; \
         (idx) < (arr).size; \
         (idx)++, (val)++)

#if REQUIRE_GCC_HACK

#define darray_enumerate_from(idx, val, arr, from) \
    if ((from) < (arr).size && (arr).item) \
    for ((idx) = (from), (val) = (void*)((char*)(arr).item + sizeof(*(arr).item)); \
         (idx) < (arr).size; \
         (idx)++, (val)++)

#else

#define darray_enumerate_from(idx, val, arr, from) \
    if ((from) < (arr).size && (arr).item) \
    for ((idx) = (from), (val) = &(arr).item[from]; \
         (idx) < (arr).size; \
         (idx)++, (val)++)

#endif

#define darray_foreach_reverse(i, arr) \
    if ((arr).size && (arr).item) \
    for ((i) = &(arr).item[(arr).size - 1]; \
         (i) >= &(arr).item[0]; \
         (i)--)
