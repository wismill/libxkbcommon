/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"

/**
 * Version 1 of `xkb_keymap_copy_options`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_keymap_copy_options_v1 {
    size_t size;
    enum xkb_keymap_copy_flags flags;
    enum xkb_keymap_format format;
    const xkb_layout_index_t *layouts;
    xkb_layout_index_t num_layouts;
    uint32_t _padding;
};

/* Ensure there is no implicit padding */
static_assert(sizeof(struct xkb_keymap_copy_options) ==
              sizeof(size_t)                          /* size        */
            + sizeof(enum xkb_keymap_copy_flags)      /* flags       */
            + sizeof(enum xkb_keymap_format)          /* format      */
            + sizeof((void *)0)                       /* layouts     */
            + sizeof(xkb_layout_index_t)              /* num_layouts */
            + sizeof(uint32_t),                       /* _padding    */
              "struct xkb_keymap_copy_options_v1 has implicit padding");

/* Current version is 1 */
static_assert(sizeof(struct xkb_keymap_copy_options) ==
              sizeof(struct xkb_keymap_copy_options_v1), "");

/** Size of the *first version* of the struct */
#define xkb_versioned_struct_size_v1(x) _Generic(      \
    (x),                                               \
    const struct xkb_keymap_copy_options *:            \
        sizeof(struct xkb_keymap_copy_options_v1)      \
)

/** Minimal *current* valid size of the struct */
#define xkb_versioned_struct_size_min(x) _Generic(     \
    (x),                                               \
    const struct xkb_keymap_copy_options *:            \
        sizeof(struct xkb_keymap_copy_options_v1)      \
)

/* V1 is the smallest struct version */
static_assert(
    xkb_versioned_struct_size_v1(((const struct xkb_keymap_copy_options *)NULL)) <=
    xkb_versioned_struct_size_min(((const struct xkb_keymap_copy_options *)NULL)),
    ""
);

/* Minimal size is lower or equal to the current size */
static_assert(
    xkb_versioned_struct_size_min(((const struct xkb_keymap_copy_options *)NULL)) <=
    sizeof(const struct xkb_keymap_copy_options),
    ""
);

#define xkb_check_keymap_copy_options_size(x) xkb_check_versioned_struct_size( \
    xkb_versioned_struct_size_v1(x),                                           \
    xkb_versioned_struct_size_min(x),                                          \
    (x)                                                                        \
)
