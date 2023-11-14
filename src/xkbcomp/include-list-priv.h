/*
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
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
 *
 * Authors: Pierre Le Marre <dev@wismill.eu>
 */


#ifndef INCLUDE_LIST_PRIV_H
#define INCLUDE_LIST_PRIV_H

#include <stdbool.h>
#include "config.h"
#include "utils.h"
#include "atom.h"
#include "darray.h"
#include "xkbcommon/xkbcommon.h"
#include "ast.h"
#include "context.h"
#include "include.h"

struct include_atom {
    xkb_atom_t file;
    xkb_atom_t map;
    bool is_map_default;
};

typedef darray(struct include_atom) includes_atoms;

typedef darray(struct include_tree) include_trees;

struct include_tree {
    struct include_atom file;
    enum xkb_file_type type;
    enum merge_mode merge;
    include_trees included;
};

void
xkb_include_tree_subtree_free(struct include_tree *tree);

struct include_tree *
xkb_get_include_tree_from_file_v1(struct xkb_context *ctx,
                                 char *file_name, char *map, FILE *file);

struct include_tree *
xkb_get_include_tree_from_names_v1(struct xkb_context *ctx,
                                   const struct xkb_rule_names *rmlvo);

bool
xkb_file_get_sections_names_from_string_v1(struct xkb_context *ctx, char *string,
                                           size_t size, const char *file_name,
                                           includes_atoms *sections);

bool
xkb_file_get_sections_names_from_file_v1(struct xkb_context *ctx,
                                         const char *file_name, FILE *file,
                                         includes_atoms *sections);

#endif
