/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <stdint.h>

#include "utils.h"
#include "util-list.h"

struct rxkb_object;

/**
 * All our objects are refcounted and are linked to iterate through them.
 * Abstract those bits away into a shared parent class so we can generate
 * most of the functions through macros.
 *
 * Object tree:
 * - `rxkb_context`
 *   - `rxkb_model`
 *   - `rxkb_layout`
 *     - `rxkb_iso639_code`
 *     - `rxkb_iso3166_code`
 *   - `rxkb_option_group`
 *     - `rxkb_option`
 *
 * #parent is `NULL` only for `rxkb_context`.
 *
 * # Ownership model
 *
 * - Containment: a child is owned by its parent’s list and is freed by the
 *   parent’s destructor. A child is never freed directly because its #refcount
 *   drops.
 * - #refcount counts *outstanding references*: one per external reference
 *   on the object (taken via a `*_ref()` call), plus one internal pin per
 *   child of the object that has an external reference of its own:
 *   - `rxkb_context` starts at 1: its creator holds an external reference;
 *   - every other object starts at 0: no external reference, and no child
 *     holds a pin.
 * - An object with #refcount > 0 holds exactly one reference on its
 *   parent. It takes that internal pin on the first external reference
 *   (0 → 1 transition) and releases it when the last reference is dropped
 *   (1 → 0 transition).
 *   Therefore, if any object in the tree has an external reference, all its
 *   ancestors have #refcount > 0 and are alive, up to the context.
 * - Only the context is ever destroyed by its #refcount reaching 0, which
 *   means no object in the tree has an external reference. At that point every
 *   child has #refcount == 0 and the context frees the whole tree.
 * - Adding a hierarchy level only requires setting `parent` at creation;
 *   ref/unref are generic and need no per-type knowledge of the parent.
 *
 * @warning Do not take internal references on objects: the 0 ↔ 1 transitions
 * are what keep the pins balanced.
 */
struct rxkb_object {
    struct rxkb_object *parent;
    uint32_t refcount;
    struct list link;
};

static_assert(HAS_TYPE(((struct rxkb_object*)0)->refcount, uint32_t),
              "refcnt must be uint32_t so that REFCOUNT_MAX is UINT32_MAX");
#define OBJECT_REFCOUNT_MAX UINT32_MAX
