/*
 * Copyright (C) 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "darray.h"
#include "utils.h"

bool
darray_heap_grow(void ** restrict data, size_t item_size,
                 darray_size_t * restrict capacity,
                 darray_size_t need)
{
    const darray_size_t need_max = darray_max_alloc(item_size);
    darray_size_t new = *capacity;

    if (new == 0) {
        new = 4;
    } else if (unlikely(need > need_max / 2)) {
        return false;
    }

    while (new < need) {
        new *= 2;
    }

    void * tmp = realloc(*data, new * item_size);
    if (unlikely(!tmp)) {
        return false;
    }

    *capacity = new;
    *data = tmp;

    return true;
}

bool
darray_heap_shrink(void ** restrict data, size_t item_size,
                   darray_size_t * restrict capacity,
                   darray_size_t size)
{
    if (size > 0) {
        void * tmp = realloc(*data, size * item_size);
        if (unlikely(!tmp)) {
            return false;
        }

        *capacity = size;
        *data = tmp;
    } else {
        free(*data);
        *capacity = 0;
        *data = NULL;
    }

    return true;
}
