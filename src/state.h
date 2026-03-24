/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include "xkbcommon/xkbcommon.h"

/**
 * Update the accessibility flags of an `xkb_machine_builder` object.
 *
 * @param[in,out] builder The `xkb_machine` builder object to modify.
 * @param[in]     affect  Accessibility flags to modify.
 * @param[in]     flags   Accessibility flags to set or unset.
 *                        Flags in @p affect but not in @p flags are cleared.
 *                        Flags outside @p affect are not changed.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine_builder
 */
static inline enum xkb_error_code
xkb_machine_builder_update_a11y_flags(
    struct xkb_machine_builder *builder,
    enum xkb_a11y_flags affect,
    enum xkb_a11y_flags flags)
{
    const struct xkb_machine_builder_a11y_update update = {
        .size = sizeof(update),
        .affect = affect,
        .flags = flags
    };
    return xkb_machine_builder_update_generic(builder, &update);
}

/**
 * Remap a modifier combination, e.g. to make `Control+Alt` act as
 * `LevelThree` (`AltGr`). This helps improve *compatibility* across platforms.
 *
 * The remapping takes effect only using
 * `xkb_machine::xkb_machine_process_key()` and under certain
 * conditions:
 *
 * - The corresponding *effective* modifiers are active.
 * - The key being processed has a type that does *not* use any of the *source*
 *   modifiers.
 * - There is no other remapping entry with the source modifiers being a
 *   superset of this entry. E.g. `Control+Alt` has priority over `Control`.
 *
 * @param[in,out] builder The `xkb_machine` builder object to modify.
 * @param[in]     source  Modifier combination to remap, using their [encoding].
 *                        Must be non-zero, unless both @p source and @p target
 *                        are 0 to clear all entries.
 * @param[in]     target  Modifier combination to remap to, using their
 *                        [encoding], or 0 to remove the entry for @p source.
 *                        If both @p source and @p target are 0, all entries are
 *                        cleared.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * Example:
 *
 * ```c
 * struct xkb_keymap *keymap = xkb_machine_builder_get_keymap(builder);
 * // Remap Control+Alt to LevelThree (AltGr)
 * const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
 * const xkb_mod_mask_t alt = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
 * const xkb_mod_mask_t level3 = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);
 * if (xkb_machine_builder_remap_mods(builder, ctrl | alt, level3)) {
 *     // handle error
 *     …
 * }
 * ```
 *
 * @since 1.14.0
 * @sa `xkb_keymap::xkb_keymap_mod_get_mask()`
 *
 * @memberof xkb_machine_builder
 *
 * [encoding]: @ref modifiers-encoding
 */
static inline enum xkb_error_code
xkb_machine_builder_remap_mods(
    struct xkb_machine_builder *builder,
    xkb_mod_mask_t source,
    xkb_mod_mask_t target
)
{
    const struct xkb_machine_builder_mods_remap_update update = {
        .size = sizeof(update),
        .source = source,
        .target = target
    };
    return xkb_machine_builder_update_generic(builder, &update);
}
