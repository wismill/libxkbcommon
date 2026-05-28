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
#include "utils-numbers.h"

bool
darray_heap_grow(void ** restrict data, size_t item_size,
                 darray_size_t * restrict capacity,
                 darray_size_t need)
{
    assert(need > *capacity);

    // const darray_size_t need_max = darray_max_alloc(item_size);

#if plouf == -1
    if (need > *capacity) {
#elif plouf == -2
    if (unlikely(need > *capacity)) {
#endif

        // need = MAX(4, need);

        // darray_size_t alloc = (need < 4) ? 4 : next_pow2(need);
        darray_size_t alloc = next_pow2(MAX(4, need));
        // if (unlikely(!alloc))
            // alloc = need;

        // darray_size_t alloc = (*capacity) ? *capacity : 4;
        // while (alloc < need)
        //     alloc *= 2;

        void * tmp = realloc(*data, alloc * item_size);
        // void * tmp = reallocarray(*data, alloc, item_size);
        if (unlikely(!tmp)) {
            return false;
        }

        *capacity = alloc;
        *data = tmp;
#if plouf < 0
    }
#endif

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
