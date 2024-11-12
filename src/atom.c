/***********************************************************
 * Copyright 1987, 1998  The Open Group
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of The Open Group shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from The Open Group.
 *
 *
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                      All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 ******************************************************************/

/************************************************************
 * Copyright 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#include "config.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "atom.h"
#include "darray.h"
#include "utils.h"
// #define ENABLE_KEYMAP_CACHE
#ifdef ENABLE_KEYMAP_CACHE_XXX
#include "hashmap.h"
#include "arena.h"
#endif
#ifdef ENABLE_KEYMAP_CACHE
#include <stdatomic.h>
#endif

// #define PLOUF
// #define PLAF

#ifndef PLAF
/* FNV-1a (http://www.isthe.com/chongo/tech/comp/fnv/). */
static inline uint32_t
hash_buf(const char *string, size_t len)
{
    uint32_t hash = 2166136261u;
#ifndef PLOUF
    for (size_t i = 0; i < (len + 1) / 2; i++) {
        hash ^= (uint8_t) string[i];
        hash *= 0x01000193;
        hash ^= (uint8_t) string[len - 1 - i];
        hash *= 0x01000193;
    }
#else
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t) string[i];
        hash *= 0x01000193;
    }
#endif
    return hash;
}

#else

/**
 * Get 32-bit Murmur3 hash.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 32-bit unsigned hash value.
 *
 * @code
 *  uint32_t hashval = qhashmurmur3_32((void*)"hello", 5);
 * @endcode
 *
 * @code
 *  MurmurHash3 was created by Austin Appleby  in 2008. The initial
 *  implementation was published in C++ and placed in the public.
 *    https://sites.google.com/site/murmurhash/
 *  Seungyoung Kim has ported its implementation into C language
 *  in 2012 and published it as a part of qLibc component.
 * @endcode
 */
static inline uint32_t
hash_buf(const char *data, size_t nbytes) {
    if (data == NULL || nbytes == 0)
        return 0;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = nbytes / 4;
    const uint32_t *blocks = (const uint32_t *) (data);
    const uint8_t *tail = (const uint8_t *) (data + (nblocks * 4));

    uint32_t h = 0;

    int i;
    uint32_t k;
    for (i = 0; i < nblocks; i++) {
        k = blocks[i];

        k *= c1;
        k = (k << 15) | (k >> (32 - 15));
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> (32 - 13));
        h = (h * 5) + 0xe6546b64;
    }

    k = 0;
    switch (nbytes & 3) {
        case 3:
            k ^= tail[2] << 16;
            /* fallthrough */
        case 2:
            k ^= tail[1] << 8;
            /* fallthrough */
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << 15) | (k >> (32 - 15));
            k *= c2;
            h ^= k;
    };

    h ^= nbytes;

    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}
#endif

#ifndef ENABLE_KEYMAP_CACHE
/*
 * The atom table is an insert-only linear probing hash table
 * mapping strings to atoms. Another array maps the atoms to
 * strings. The atom value is the position in the strings array.
 */
struct atom_table {
    xkb_atom_t *index;
    size_t index_size;
    darray(char *) strings;

#ifdef COLLISIONS_STATS
    size_t collisions;
    size_t queries;
#endif
};

struct atom_table *
atom_table_new(void)
{
    struct atom_table *table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    darray_init(table->strings);
    darray_append(table->strings, NULL);
#ifdef ENABLE_KEYMAP_CACHE
    darray_resize0(table->strings, size);
    table->index_size = size * 10 / 8;
#else
    table->index_size = 4;
#endif
    table->index = calloc(table->index_size, sizeof(*table->index));

    return table;
}

void
atom_table_free(struct atom_table *table)
{
    if (!table)
        return;

    char **string;
    // FIXME: remove debug
    // fprintf(stderr, "~~~ Atom table entries: %u\n", darray_size(table->strings));
// FIXME: remove debug
#ifdef COLLISIONS_STATS
    double collisions = table->collisions * 1.0 / table->queries;
    fprintf(stderr, "~~~ Atom table collisions: %zu/%zu (%f)\n", table->collisions, table->queries, collisions);
#endif
    darray_foreach(string, table->strings)
        free(*string);
    darray_free(table->strings);
    free(table->index);
    free(table);
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    assert(atom < darray_size(table->strings));
    return darray_item(table->strings, atom);
}

xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len, bool add)
{
#ifdef COLLISIONS_STATS
    table->queries++;
#endif
    if (darray_size(table->strings) > 0.80 * table->index_size) {
        table->index_size *= 2;
        table->index = realloc(table->index, table->index_size * sizeof(*table->index));
        memset(table->index, 0, table->index_size * sizeof(*table->index));
        for (size_t j = 1; j < darray_size(table->strings); j++) {
            const char *s = darray_item(table->strings, j);
            uint32_t hash = hash_buf(s, strlen(s));
            for (size_t i = 0; i < table->index_size; i++) {
                size_t index_pos = (hash + i) & (table->index_size - 1);
                if (index_pos == 0)
                    continue;

                xkb_atom_t atom = table->index[index_pos];
                if (atom == XKB_ATOM_NONE) {
                    table->index[index_pos] = j;
                    break;
                }
            }
        }
    }

    uint32_t hash = hash_buf(string, len);
    for (size_t i = 0; i < table->index_size; i++) {
        size_t index_pos = (hash + i) & (table->index_size - 1);
        if (index_pos == 0)
            continue;

        xkb_atom_t existing_atom = table->index[index_pos];
        if (existing_atom == XKB_ATOM_NONE) {
            if (add) {
                xkb_atom_t new_atom = darray_size(table->strings);
                darray_append(table->strings, strndup(string, len));
                table->index[index_pos] = new_atom;
                return new_atom;
            } else {
                return XKB_ATOM_NONE;
            }
        }

        const char *existing_value = darray_item(table->strings, existing_atom);
        if (strncmp(existing_value, string, len) == 0 && existing_value[len] == '\0')
            return existing_atom;
        // fprintf(stderr, "~~~ Collision: expected %s, got: %s\n", string, existing_value);
#ifdef COLLISIONS_STATS
        table->collisions++;
#endif
    }

    assert(!"couldn't find an empty slot during probing");
}

#elif 0

MAKE_HASHMAP(hashmap, char *);

struct atom_table {
    arena arena;
    hashmap map;
};

struct atom_table *
atom_table_new(size_t size)
{
    struct atom_table *table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    table->arena.beginning = calloc(size, sizeof(hashmap));
    table->arena.end = table->arena.beginning + size * sizeof(hashmap);
    return table;
}

static void
atom_table_free_strings(hashmap *map)
{
    if (!map)
        return;
    free(map->key.buf);
    for (short k = 0; k < ARRAY_SIZE(map->child); k++) {
        atom_table_free_strings(map->child[k]);
    }
}

void
atom_table_free(struct atom_table *table)
{
    if (!table)
        return;

    atom_table_free_strings(&table->map);
    free(table->arena.beginning);
    free(table);
}


xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len, bool add)
{
    str key = {
        .buf = string,
        .len = strlen(string)
    };
    char **s = NULL;
    if (add) {
        hashmap *m = upsert(&table->map, key, &table->arena);
        if (m && m->value == NULL) {
            char *buf = strndup(key.buf, key.len);
            if (atomic_compare_exchange_strong_explicit(
                &m->value, s, buf, memory_order_release, memory_order_acquire)) {
                if (!atomic_compare_exchange_strong_explicit(
                    &m->key.buf, s, buf, memory_order_release, memory_order_acquire))
                    free(buf);
            } else {
                free(buf);
            }
            s = &m->value;
        }
    } else {
        s = lookup(&table->map, key);
    }
    return (s)
        ? (xkb_atom_t)(s - table->arena.beginning)
        : XKB_ATOM_NONE;
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    assert(atom < table->arena.end - table->arena.beginning);
    return (const char*)*(table->arena.beginning + atom);
}

#else

struct atom_table {
    size_t size;
    char **strings;
};

struct atom_table *
atom_table_new(size_t size_log2)
{
    struct atom_table *table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;
    size_t size = 1u << size_log2;
    table->size = size;
    table->strings = calloc(size, sizeof(char*));
    return table;
}

void
atom_table_free(struct atom_table *table)
{
    if (!table)
        return;

    size_t count = 0;
    for (size_t k = 0 ; k < table->size; k++) {
        if (table->strings[k]) {
            free(table->strings[k]);
            count++;
        }
    }
    // FIXME: remove debug
    fprintf(stderr, "Atom entries: %zu\n", count);
    free(table->strings);
    free(table);
}


xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len, bool add)
{

    uint32_t hash = hash_buf(string, len);
    char *new_value = NULL;
    // FIXME: remove debug
    // fprintf(stderr, "Looking for: %s (%d)\n", string, add);
    for (size_t i = 0; i < table->size; i++) {
        xkb_atom_t index_pos = (hash + i) & (table->size - 1);
        if (index_pos == XKB_ATOM_NONE)
            continue;

        char *existing_value = atomic_load_explicit(&table->strings[index_pos],
                                                    memory_order_acquire);

        /* Check if there is a value at the current index */
        if (!existing_value) {
            /* No value defined. Check if we are allowed to add one */
            if (!add)
                return XKB_ATOM_NONE;

            /*
             * Prepare addition: duplicate string.
             * Warning: This may not be our first attempt!
             */
            if (!new_value)
                new_value = strndup(string, len);
            if (!new_value) {
                /* Failed memory allocation */
                // FIXME: error handling
                return XKB_ATOM_NONE;
            }
            /* Try to register a new entry */
            if (atomic_compare_exchange_strong_explicit(
                    &table->strings[index_pos], &existing_value, new_value,
                    memory_order_release, memory_order_acquire)) {
                // FIXME: remove debug
                // fprintf(stderr, "Yes! %s\n", existing_value);
                return index_pos;
            }
#ifdef COLLISIONS_STATS
            else {
                fprintf(stderr, "Failed! Expected %s, got: %s\n", new_value, table->strings[index_pos]);
            }
#endif
            /*
             * We were not fast enough to take the spot.
             * But maybe the newly added value is our value, so read it again.
             */
            existing_value = atomic_load_explicit(&table->strings[index_pos],
                                                  memory_order_acquire);
        }

        /* Check the value are equal */
        if (strncmp(existing_value, string, len) == 0 &&
            existing_value[len] == '\0') {
            /* We may have tried unsuccessfully to add the string */
            free(new_value);
            // FIXME: remove debug
            // fprintf(stderr, "Found it! %s\n", string);
            return index_pos;
        }
        /* Hash collision: try next atom */
        // FIXME: remove debug
        // fprintf(stderr, "Oh no! Collision for %s: %s\n", string, existing_value);
    }

    free(new_value);
#ifdef COLLISIONS_STATS
    fprintf(stderr, "ERROR: not enough slot in atom table (capacity: %zu)\n", table->size);
    size_t count = 0;
    for (size_t i = 0; i < table->size; i++) {
        xkb_atom_t index_pos = (hash + i) & (table->size - 1);
        if (table->strings[i]) {
            fprintf(stderr, "%zu. %s (%u)\n", i, table->strings[i], index_pos);
            count++;
        }
        else if (index_pos != 0 && !table->strings[index_pos]) {
            fprintf(stderr, "%zu. (%u) !!!\n", i, index_pos);
        }
    }
    fprintf(stderr, "~~~ Count: %zu\n", count);
    fflush(stderr);
#endif
    assert(!"couldn't find an empty slot during probing");
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    assert(atom < table->size);
    return table->strings[atom];
}

#endif
