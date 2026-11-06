/*
 * SPDX-License-Identifier: MIT
 *
 * Adapted from utf8proc_iterate() of utf8proc (src/utf8proc.c),
 * https://github.com/JuliaStrings/utf8proc, which is distributed under the
 * following license:
 *
 * Copyright (c) 2014-2021 Steven G. Johnson, Jiahao Chen, Peter Colberg,
 * Tony Kelman, Scott P. Jones, and other contributors.
 * Copyright (c) 2009 Public Software Group e. V., Berlin, Germany
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utf8-decoding.h"
#include "utils.h"

/* Doc only: keep this private */
enum xkb_utf8_error {
    XKB_UTF8_INVALID = -1,
};

/** Check that a byte is a UTF-8 continuation byte (0b10xxxxxx) */
static bool
is_utf8_continuation(uint32_t c)
{
    return ((c & 0xc0) == 0x80);
}

/*
 * Reads the next UTF-8 sequence in a string.
 *
 * Only well-formed sequences are accepted: no overlong encodings, no
 * surrogates and no code point beyond U+10FFFF. Otherwise return
 * INVALID_UTF8_CODE_POINT and do not set `cp`.
 * Never reads more than `max_size` bytes.
 *
 */
ssize_t
xkb_utf8_iterate(const char *s, size_t max_size, uint32_t *cp)
{
    if (!max_size)
        return 0;

    uint32_t b0 = (uint8_t)s[0];

    if (b0 < 0x80) {
        *cp = b0;
        return 1;
    }

    /* Must be between 0xc2 and 0xf4 inclusive to be valid */
    if (b0 - 0xc2 > 0xf4 - 0xc2)
        return XKB_UTF8_INVALID;

    if (b0 < 0xe0) {
        /* 2-byte sequence: must have a valid continuation byte */
        if (max_size < 2 || !is_utf8_continuation(s[1]))
            return XKB_UTF8_INVALID;
        *cp = ((b0 & 0x1f) << 6) | ((uint8_t)s[1] & 0x3f);
        return 2;
    }

    if (b0 < 0xf0) {
        /* 3-byte sequence: must have 2 valid continuation bytes */
        if (max_size < 3 ||
            !is_utf8_continuation(s[1]) ||
            !is_utf8_continuation(s[2]))
        {
            return XKB_UTF8_INVALID;
        }
        /* Check for surrogates */
        if (b0 == 0xed && (uint8_t)s[1] > 0x9f)
            return XKB_UTF8_INVALID;
        b0 = (
            ((b0 & 0xf) << 12) |
            (((uint8_t)s[1] & 0x3f) << 6) |
            ((uint8_t)s[2] & 0x3f)
        );
        /* Check for overlong encodings */
        if (b0 < 0x800)
            return XKB_UTF8_INVALID;
        *cp = b0;
        return 3;
    }

    /* 4-byte sequence: must have 3 valid continuation bytes */
    if (max_size < 4 ||
        !is_utf8_continuation(s[1]) ||
        !is_utf8_continuation(s[2]) ||
        !is_utf8_continuation(s[3]))
    {
        return XKB_UTF8_INVALID;
    }
    /* Make sure it is in the correct range (U+10000..U+10FFFF) */
    if (b0 == 0xf0) {
        if ((uint8_t)s[1] < 0x90)
            return XKB_UTF8_INVALID;
    } else if (b0 == 0xf4) {
        if ((uint8_t)s[1] > 0x8f)
            return XKB_UTF8_INVALID;
    }
    *cp = (
        ((b0 & 7) << 18) |
        (((uint8_t)s[1] & 0x3f) << 12) |
        (((uint8_t)s[2] & 0x3f) << 6) |
        ((uint8_t)s[3] & 0x3f)
    );
    return 4;
}
