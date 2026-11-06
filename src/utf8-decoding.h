/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include <stddef.h>
#include <stdint.h>

#include "utils.h"

ssize_t
xkb_utf8_iterate(const char *s, size_t max_size, uint32_t *cp);
