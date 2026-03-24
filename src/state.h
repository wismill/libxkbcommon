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
