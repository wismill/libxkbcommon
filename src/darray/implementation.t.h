/*
 * Copyright (C) 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "utils.h"
#include "darray.h"

/*
 * Utils
 */

#define DARRAY_CONCAT_(x,y) x ## y
#define DARRAY_CONCAT(x,y) DARRAY_CONCAT_(x,y)

#ifndef DARRAY_TYPE
#   ifndef DARRAY_PREFIX
#       error "DARRAY_TYPE nor DARRAY_PREFIX is defined"
#   else
#       define DARRAY_TYPE DARRAY_CONCAT(DARRAY_PREFIX, _t)
#   endif
#endif

#ifndef DARRAY_ITEM_TYPE
#   error "DARRAY_ITEM_TYPE is not defined"
#endif

#ifndef DARRAY_PREFIX
#   define DARRAY_PREFIX DARRAY_TYPE
#endif

/*
 * Types
 */

#define darray_t DARRAY_TYPE
#define darray_item_t DARRAY_CONCAT(DARRAY_PREFIX, _item_t)

typedef DARRAY_ITEM_TYPE darray_item_t;

typedef struct {
    /** Count of allocated items */
    darray_size_t alloc;
    /** Current size */
    darray_size_t size;
    /** Array of items */
    darray_item_t *item ATTR_COUNTED_BY(size);
} darray_t;

/*
 * Functions
 */

/*** Insertion (single item) ***/

#define darray_append__ DARRAY_CONCAT(DARRAY_PREFIX, _append)
#define darray_insert DARRAY_CONCAT(DARRAY_PREFIX, _insert)

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_append__(darray_t * restrict arr, darray_item_t item)
{
    if (!darray_resize(*arr, arr->size + 1))
        return false;

    arr->item[arr->size - 1] = item;

    return true;
}

ATTRIBS(NODISCARD, MAYBE_UNUSED) static inline bool
darray_insert(darray_t * restrict arr, darray_size_t i, darray_item_t item)
{
    if (!darray_resize(*arr, arr->size + 1))
        return false;

    memmove(
        arr->item + i + 1,
        arr->item + i,
        (arr->size - i - 1) * sizeof(*arr->item)
    );

    arr->item[i] = item;

    return true;
}

/*** Insertion (multiple items) ***/

#define darray_append_items DARRAY_CONCAT(DARRAY_PREFIX, _append_items)
#define darray_from_items DARRAY_CONCAT(DARRAY_PREFIX, _from_items)
#define darray_copy DARRAY_CONCAT(DARRAY_PREFIX, _copy)
#define darray_concat DARRAY_CONCAT(DARRAY_PREFIX, _concat)

ATTRIBS(NODISCARD, MAYBE_UNUSED) static inline bool
darray_append_items(darray_t * restrict arr,
                    const darray_item_t * restrict items, darray_size_t count)
{
    const darray_size_t old_size = arr->size;

    if (!darray_resize(*arr, arr->size + count))
        return false;

    memcpy(arr->item + old_size, items, count * sizeof(*arr->item));

    return true;
}

ATTRIBS(NODISCARD, MAYBE_UNUSED) static inline bool
darray_from_items(darray_t * restrict arr,
                  const darray_item_t * restrict items, darray_size_t count)
{
    if (arr->alloc ||
        !darray_resize(*arr, count)) {
        return false;
    }

    if (count) {
        memcpy(arr->item, items, count * sizeof(*arr->item));
    }

    return true;
}

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_copy(darray_t * restrict dest, const darray_t * restrict source)
{
    return darray_from_items(dest, source->item, source->size);
}

ATTRIBS(NODISCARD, MAYBE_UNUSED, ALWAYS_INLINE) static inline bool
darray_concat(darray_t * restrict dest, const darray_t * restrict source)
{
    return !source->size || darray_append_items(dest, source->item, source->size);
}

/*
 * Undefine
 */

#undef DARRAY_TYPE
#undef DARRAY_PREFIX
#undef DARRAY_ITEM_TYPE

#undef DARRAY_CONCAT_
#undef DARRAY_CONCAT

#undef darray_t
#undef darray_item_t

#undef darray_append
#undef darray_insert

#undef darray_append_items
#undef darray_from_items
#undef darray_copy
#undef darray_concat
