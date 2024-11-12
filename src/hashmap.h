/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Thread-safe, lock-free hash-trie
 *
 * The upsert function is like the "standard" upsert except that it uses
 * acquire/release atomics to navigate and update trie references.
 *
 * Adapted from (public domain):
 * https://github.com/skeeto/scratch/blob/5cd94b43b69673468e0cf2fd69ce52d79471a63e/misc/concurrent-hash-trie.c
 *
 * Blog post:
 * https://nullprogram.com/blog/2023/09/30/
 */

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "arena.h"

#define MAKE_HASHMAP(map, type)                                        \
    typedef struct map map;                                            \
    struct map {                                                       \
        map *child[4];                                                 \
        str  key;                                                      \
        type value;                                                    \
    };                                                                 \
    static type*                                                       \
    upsert(map **m, str key, arena *a)                                 \
    {                                                                  \
        for (uint64_t h = hash64(key);; h <<= 2) {                     \
            map *n = atomic_load_explicit(m, memory_order_acquire);    \
            if (!n) {                                                  \
                if (!a) {                                              \
                    return 0;                                          \
                }                                                      \
                arena rollback = *a;                                   \
                map *new = new(a, map, 1);                             \
                new->key = key;                                        \
                if (atomic_compare_exchange_strong_explicit(           \
                        m, &n, new,                                    \
                        memory_order_release, memory_order_acquire)) { \
                    return &new->value;                                \
                }                                                      \
                *a = rollback;                                         \
            }                                                          \
            if (equals(n->key, key)) {                                 \
                return &n->value;                                      \
            }                                                          \
            m = n->child + (h >> 62);                                  \
        }                                                              \
    }

#define new(a, t, n)  (t *)arena_alloc(a, sizeof(t), _Alignof(t), n, true)

typedef unsigned char byte;

typedef struct {
    byte     *buf;
    ptrdiff_t len;
} str;

static str
copyinto(str s, arena *a)
{
    str r = {0};
    r.buf = new(a, byte, s.len);
    memcpy(r.buf, s.buf, s.len);
    r.len = s.len;
    return r;
}

static bool
equals(str a, str b)
{
    return a.len==b.len && !memcmp(a.buf, b.buf, a.len);
}

static uint64_t
hash64(str s)
{
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < s.len; i++) {
        h ^= s.buf[i];
        h *= 1111111111111111111u;
    }
    return h ^ h>>32;
}

typedef struct map map;
struct map {
    map *child[4];
    str  key;
    int  value;
};

typedef struct {
    arena arena;
    map **root;
    int   start;
    int   stop;
} context;

#endif
