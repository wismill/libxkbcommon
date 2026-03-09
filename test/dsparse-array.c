/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "src/dsparse-array.h"
#include "src/utils.h"

#define assert_printf(cond, ...) do {                      \
    const bool __cond = (cond);                            \
    if (!__cond) {                                         \
       fprintf(stderr, "Assertion failure: " __VA_ARGS__); \
       assert(__cond);                                     \
    }} while (0)

#define assert_eq(test_name, expected, got, format, ...) \
    assert_printf(expected == got, \
                  test_name ". Expected " format ", got: " format "\n", \
                  ##__VA_ARGS__, expected, got)

/* -------------------------------------------------------------------------
 * Tests
 * ------------------------------------------------------------------------- */

static void
test_next_pow2(void)
{
    struct {
        uint8_t input;
        uint8_t output;
    } tests[] = {
        { 0, 1 },
        { 1, 1 },
        { 2, 2 },
        { 3, 4 },
        { 4, 4 },
        { 5, 8 },
        { 6, 8 },
        { 7, 8 },
        { 8, 8 },
        { 9, 16 },
        { 10, 16 },
        { 15, 16 },
        { 16, 16 },
    };
    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        assert_eq("input: %u; ", tests[t].output, next_pow2(tests[t].input),
                  "%u", tests[t].input);
    }
}

/* int sparse array: 32-bit mask, 8-bit count */
dsparse_array_define(int_sparse_t, int, uint32_t, uint8_t, true, one, multi);

static void
test_empty(void)
{
    fprintf(stderr, "=== %s ===\n", __func__);
    int_sparse_t sa = dsparse_array_new();

    assert(sa.count == 0);
    assert(sa.mask  == 0);

    int val = 99;
    assert(!int_sparse_t_get(&sa, 0, &val));
    assert(val == 99);
    assert(!int_sparse_t_get(&sa, UINT8_MAX - 1, &val));
    assert(val == 99);

    int_sparse_t_free(&sa);
    assert(sa.count == 0);
}

static void
test_one_element(void)
{
    fprintf(stderr, "=== %s (no heap alloc) ===\n", __func__);
    int_sparse_t sa = dsparse_array_new();

    assert(int_sparse_t_insert(&sa, 5, 42));
    assert(sa.count == 1);
    assert(sa.mask == (UINT32_C(1) << 5));

    int val = 0;
    assert(int_sparse_t_get(&sa, 5, &val));
    assert(val == 42);
    assert(!int_sparse_t_get(&sa, 0, NULL)); /* absent index */

    /* overwrite */
    assert(int_sparse_t_insert(&sa, 5, -7));
    assert(sa.count == 1);
    assert(sa.mask == (UINT32_C(1) << 5));
    assert(sa.one == -7);
    assert(int_sparse_t_get(&sa, 5, &val) && val == -7);

    int_sparse_t_free(&sa);
}

static void
test_multiple_elements(void)
{
    fprintf(stderr, "=== %s (heap promoted) ===\n", __func__);
    int_sparse_t sa = dsparse_array_new();

    const int v0 = -1, v3 = 30, v5 = 50, v6 = 60, v7 = 70;

    assert(int_sparse_t_insert(&sa, 3, v3));
    assert(int_sparse_t_insert(&sa, 6, v6));

    assert(int_sparse_t_count(&sa) == 2);
    assert(sa.alloc == 2);
    assert(sa.mask  == UINT32_C(0x48));

    int v = 0;
    assert(int_sparse_t_get(&sa, 3, &v) && v == v3);
    assert(int_sparse_t_get(&sa, 6, &v) && v == v6);

    /* heap array should be rank-ordered: index 3 < index 7 */
    assert(sa.multi[0] == v3);
    assert(sa.multi[1] == v6);

    /* insert between existing ranks */
    assert(int_sparse_t_insert(&sa, 5, v5));
    assert(int_sparse_t_count(&sa) == 3);
    assert(sa.alloc == 4);
    assert(int_sparse_t_get(&sa, 5, &v) && v == v5);
    assert(sa.multi[0] == v3);
    assert(sa.multi[1] == v5);
    assert(sa.multi[2] == v6);

    /* insert after existing ranks */
    assert(int_sparse_t_insert(&sa, 7, v7));
    assert(int_sparse_t_count(&sa) == 4);
    assert(sa.alloc == 4);
    assert(int_sparse_t_get(&sa, 7, &v) && v == v7);
    assert(sa.multi[0] == v3);
    assert(sa.multi[1] == v5);
    assert(sa.multi[2] == v6);
    assert(sa.multi[3] == v7);

    int_sparse_t_free(&sa);
    assert(int_sparse_t_count(&sa) == 0);
    assert(sa.count == 0);
    assert(sa.multi == NULL);

    /* unordered */
    assert(int_sparse_t_insert(&sa, 7, v7));
    assert(int_sparse_t_insert(&sa, 5, v5));
    assert(sa.multi[0] == v5);
    assert(sa.multi[1] == v7);
    assert(int_sparse_t_get(&sa, 5, &v) && v == v5);
    assert(int_sparse_t_get(&sa, 7, &v) && v == v7);

    assert(int_sparse_t_insert(&sa, 0, v0));
    assert(sa.multi[0] == v0);
    assert(sa.multi[1] == v5);
    assert(sa.multi[2] == v7);
    assert(int_sparse_t_get(&sa, 0, &v) && v == v0);
    assert(int_sparse_t_get(&sa, 5, &v) && v == v5);
    assert(int_sparse_t_get(&sa, 7, &v) && v == v7);

    assert(int_sparse_t_insert(&sa, 3, v3));
    assert(sa.multi[0] == v0);
    assert(sa.multi[1] == v3);
    assert(sa.multi[2] == v5);
    assert(sa.multi[3] == v7);
    assert(int_sparse_t_get(&sa, 0, &v) && v == v0);
    assert(int_sparse_t_get(&sa, 3, &v) && v == v3);
    assert(int_sparse_t_get(&sa, 5, &v) && v == v5);
    assert(int_sparse_t_get(&sa, 7, &v) && v == v7);

    int_sparse_t_free(&sa);
}

static void
test_invalid_index(void)
{
    fprintf(stderr, "=== %s ===\n", __func__);
    int_sparse_t sa = dsparse_array_new();

    static const uint8_t max_legal_index =
        (uint8_t)(sizeof(((int_sparse_t *)NULL)->mask) * CHAR_BIT - 1);
    static_assert((uint8_t)(max_legal_index + 1) > max_legal_index,
                  "no overflow");

    /* Maximum legal index + 1 */
    assert(!int_sparse_t_insert(&sa, max_legal_index + 1, 1));
    assert(int_sparse_t_count(&sa) == 0);
    assert(sa.count == 0);
    assert(sa.mask == 0);
    assert(sa.multi == NULL);

    /* Maximum of the index type */
    assert(UINT8_MAX > max_legal_index);
    assert(!int_sparse_t_insert(&sa, UINT8_MAX, 1));
    assert(int_sparse_t_count(&sa) == 0);
    assert(sa.count == 0);
    assert(sa.mask == 0);
    assert(sa.multi == NULL);

    int val = 1;
    assert(int_sparse_t_insert(&sa, max_legal_index, val));
    assert(int_sparse_t_count(&sa) == 1);
    assert(sa.count == 1);
    assert(sa.mask == (UINT32_C(1) << max_legal_index));
    assert(sa.one == val);

    assert(!int_sparse_t_get(&sa, max_legal_index + 1, &val));
    assert(val == 1);
    assert(!int_sparse_t_get(&sa, UINT8_MAX, &val));
    assert(val == 1);

    int_sparse_t_free(&sa);
}

static void
test_merge_disjoint(void)
{
    fprintf(stderr, "=== %s ===\n", __func__);
    int_sparse_t a = dsparse_array_new();
    int_sparse_t b = dsparse_array_new();

    assert(int_sparse_t_insert(&a, 1, 10));
    assert(int_sparse_t_insert(&a, 2, 20));
    assert(int_sparse_t_count(&a) == 2);
    assert(a.alloc == 2);

    assert(int_sparse_t_insert(&b, 4, 40));
    assert(int_sparse_t_insert(&b, 6, 60));
    assert(int_sparse_t_insert(&b, 7, 70));
    assert(int_sparse_t_count(&b) == 3);
    assert(b.alloc == 4);
    assert(int_sparse_t_merge(&a, &b));
    assert(int_sparse_t_count(&a) == 5);
    assert_eq("", 5, a.alloc, "%u");
    assert(a.mask == UINT32_C(0xd6));

    int v = 0;
    assert(int_sparse_t_get(&a, 1, &v) && v == 10);
    assert(int_sparse_t_get(&a, 2, &v) && v == 20);
    assert(int_sparse_t_get(&a, 4, &v) && v == 40);
    assert(int_sparse_t_get(&a, 6, &v) && v == 60);
    assert(int_sparse_t_get(&a, 7, &v) && v == 70);

    /* src unchanged */
    assert(int_sparse_t_count(&b) == 3);
    assert(b.alloc == 4);

    int_sparse_t_free(&a);
    int_sparse_t_free(&b);
}

static void
test_merge_overlap(void)
{
    fprintf(stderr, "=== %s (src wins on collision) ===\n", __func__);
    int_sparse_t a = dsparse_array_new();
    int_sparse_t b = dsparse_array_new();

    assert(int_sparse_t_insert(&a, 0, 1));
    assert(int_sparse_t_insert(&a, 2, 3));

    assert(int_sparse_t_insert(&b, 2, 99)); /* overlaps index 2 */
    assert(int_sparse_t_insert(&b, 3, 4));

    assert(int_sparse_t_merge(&a, &b));
    assert(int_sparse_t_count(&a) == 3);
    assert(a.alloc == 3);

    int v = 0;
    assert(int_sparse_t_get(&a, 0, &v) && v == 1);
    assert(int_sparse_t_get(&a, 2, &v) && v == 99); /* src wins */
    assert(int_sparse_t_get(&a, 3, &v) && v == 4);

    int_sparse_t_free(&a);
    int_sparse_t_free(&b);
}

static void
test_merge_some_empty(void)
{
    fprintf(stderr, "=== %s ===\n", __func__);
    int_sparse_t a = dsparse_array_new();
    int_sparse_t b = dsparse_array_new();

    assert(int_sparse_t_insert(&b, 10, 777));

    assert(int_sparse_t_merge(&a, &b));
    assert(int_sparse_t_count(&a) == 1);
    assert(int_sparse_t_count(&b) == 1);
    assert(a.count == b.count && a.count == 1);
    assert(a.mask == b.mask && a.mask == (UINT32_C(1) << 10));
    int v = 0;
    assert(int_sparse_t_get(&a, 10, &v) && v == 777);

    int_sparse_t_free(&b);
    assert(int_sparse_t_merge(&a, &b));
    /* unchanged */
    assert(int_sparse_t_count(&a) == 1);
    assert(a.count == 1);
    assert(a.mask == (UINT32_C(1) << 10));
    assert(int_sparse_t_get(&a, 10, &v) && v == 777);

    int_sparse_t_free(&a);
    int_sparse_t_free(&b);
}

static void
test_zero_merge(void)
{
    fprintf(stderr, "=== %s ===\n", __func__);
    int_sparse_t a = dsparse_array_new();
    int_sparse_t b = dsparse_array_new();
    assert(int_sparse_t_merge(&a, &b));
    assert(int_sparse_t_count(&a) == 0);
    assert(a.count == 0);
    assert(a.mask == 0);
    int_sparse_t_free(&a);
    int_sparse_t_free(&b);
}

/* Check _Generic routing with uint8_t mask */
dsparse_array_define(u8mask_sparse_t, int, uint8_t, uint8_t, false, one, multi);

static void
test_u8_mask(void)
{
    fprintf(stderr, "=== %s (_Generic -> _sparse_popcount8) ===\n", __func__);
    u8mask_sparse_t sa = dsparse_array_new();

    assert(u8mask_sparse_t_insert(&sa, 0, 10));
    assert(u8mask_sparse_t_insert(&sa, 7, 70));
    assert(u8mask_sparse_t_count(&sa) == 2);
    assert(sa.alloc == 2);

    int v = 0;
    assert(u8mask_sparse_t_get(&sa, 0, &v) && v == 10);
    assert(u8mask_sparse_t_get(&sa, 7, &v) && v == 70);

    assert(u8mask_sparse_t_insert(&sa, 5, 50));
    assert(u8mask_sparse_t_count(&sa) == 3);
    assert(sa.alloc == 4);
    assert(u8mask_sparse_t_get(&sa, 0, &v) && v == 10);
    assert(u8mask_sparse_t_get(&sa, 5, &v) && v == 50);
    assert(u8mask_sparse_t_get(&sa, 7, &v) && v == 70);

    u8mask_sparse_t_free(&sa);
}

static void
random_unique(uint8_t buf[8], uint8_t *count)
{
    /* Prefill */
    for (uint8_t i = 0; i < 8; i++)
        buf[i] = i;

    /* Fisher-Yates shuffle in place */
    for (uint8_t i = 7; i > 0; i--) {
        uint8_t j = (uint8_t)(rand() % (i + 1));
        uint8_t t = buf[i]; buf[i] = buf[j]; buf[j] = t;
    }

    *count = (uint8_t)(1 + rand() % 8);
}

#define FOREACH_LOOPS 1000

static void
test_foreach(void)
{
    fprintf(stderr, "=== %s ===\n", __func__);
    u8mask_sparse_t sa = dsparse_array_new();

    /* Empty array: foreach must not execute the body */
    uint8_t bit   = UINT8_MAX;
    const int *value = NULL;
    dsparse_array_foreach(u8mask_sparse_t, int, uint8_t, uint8_t,
                          one, multi, sa, value, bit);
    assert(bit   == UINT8_MAX);
    assert(value == NULL);

    for (unsigned loop = 0; loop < FOREACH_LOOPS; loop++) {
        /* Pick a random insertion order for bits 0-7 */
        uint8_t order[8];
        uint8_t count;
        random_unique(order, &count);

        static const int ref[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

        /* Insert in random order, verifying iteration after each insertion */
        for (uint8_t e = 0; e < count; e++) {
            uint8_t ins_bit = order[e];
            assert(u8mask_sparse_t_insert(&sa, ins_bit, ref[ins_bit]));

            /* Collect the bits inserted so far and sort them */
            uint8_t expected_bits[8];
            uint8_t n = e + 1;
            for (uint8_t i = 0; i < n; i++)
                expected_bits[i] = order[i];
            /* insertion sort — n ≤ 8, negligible cost */
            for (uint8_t i = 1; i < n; i++) {
                uint8_t key = expected_bits[i];
                int8_t j = (int8_t)((int8_t)i - 1);
                while (j >= 0 && expected_bits[j] > key) {
                    expected_bits[j + 1] = expected_bits[j];
                    j--;
                }
                expected_bits[j + 1] = key;
            }

            /* Iterate and verify order and values */
            uint8_t idx = 0;
            bit   = 0;
            value = NULL;
            dsparse_array_foreach(u8mask_sparse_t, int, uint8_t, uint8_t,
                                  one, multi, sa, value, bit) {
                assert(idx < n);
                assert_eq("%d:%d (bit)",
                          expected_bits[idx], bit, "%u", loop, e);
                assert_eq("%d:%d (value)",
                          ref[expected_bits[idx]], *value, "%d", loop, e);
                idx++;
            }
            assert(idx == n);
        }

        u8mask_sparse_t_free(&sa);
    }
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    /* Initialize pseudo-random generator with program arg or current time */
    unsigned int seed;
    if (argc >= 2 && !streq(argv[1], "-")) {
        seed = (unsigned int) atoi(argv[1]);
    } else {
        seed = (unsigned int) time(NULL);
    }
    fprintf(stderr, "Seed for the pseudo-random generator: %u\n", seed);
    srand(seed);

    test_next_pow2();
    test_empty();
    test_one_element();
    test_multiple_elements();
    test_invalid_index();
    test_merge_disjoint();
    test_merge_overlap();
    test_merge_some_empty();
    test_zero_merge();

    test_u8_mask();
    test_foreach();

    return EXIT_SUCCESS;
}
