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

#include "include-list-priv.h"

#include "xkbcomp-priv.h"
#include "rules.h"

void
xkb_include_tree_subtree_free(struct include_tree *tree)
{
    struct include_tree *subtree;
    darray_foreach(subtree, tree->included) {
        xkb_include_tree_subtree_free(subtree);
    }
    darray_free(tree->included);
}

static void
create_include_atom(struct xkb_context *ctx, XkbFile *file, struct include_atom *atom)
{
    atom->file = isempty(file->file_name)
        ? XKB_ATOM_NONE
        : xkb_atom_intern(ctx, file->file_name, strlen(file->file_name));
    atom->map = isempty(file->name)
        ? XKB_ATOM_NONE
        : xkb_atom_intern(ctx, file->name, strlen(file->name));
    atom->is_map_default = !!(file->flags & MAP_IS_DEFAULT);
}

static bool
xkb_get_include_tree(struct xkb_context *ctx, include_trees *includes, XkbFile *file)
{
    for (ParseCommon *stmt = file->defs; stmt; stmt = stmt->next) {
        if (stmt->type != STMT_INCLUDE) {
            continue;
        }
        /* TODO: check recursive inputs */
        for (IncludeStmt *include = (IncludeStmt *) stmt; include; include = include->next_incl) {
            XkbFile *included_file = ProcessIncludeFile(ctx, include, file->file_type);

            if (!included_file) {
                FreeXkbFile(included_file);
                continue;
            }

            struct include_atom atom;
            create_include_atom(ctx, included_file, &atom);

            include_trees included_includes = darray_new();

            if (!xkb_get_include_tree(ctx, &included_includes, included_file)) {
                darray_free(included_includes);
                FreeXkbFile(included_file);
                continue;
            } else {
                struct include_tree included_tree = {
                    .file = atom,
                    .type = file->file_type,
                    .merge = include->merge,
                    .included = included_includes
                };
                darray_init(included_includes);
                darray_append(*includes, included_tree);
                darray_init(included_tree.included);
            }

            FreeXkbFile(included_file);
        }
    }
    return true;
}

static struct include_tree *
xkb_get_component_include_tree(struct xkb_context *ctx, XkbFile *file)
{
    struct include_atom atom;
    struct include_tree *tree;

    create_include_atom(ctx, file, &atom);
    include_trees includes = darray_new();
    if (xkb_get_include_tree(ctx, &includes, file)) {
        tree = calloc(1, sizeof(*tree));
        tree->file = atom;
        tree->type = file->file_type;
        tree->included = includes;
        darray_init(includes);
        return tree;
    } else {
        return NULL;
    }
}

static struct include_tree *
xkb_get_keymap_include_tree(struct xkb_context *ctx, XkbFile *file)
{
    struct include_atom atom;
    struct include_tree *tree;

    create_include_atom(ctx, file, &atom);
    tree = calloc(1, sizeof(*tree));
    tree->file = atom;
    tree->type = file->file_type;

    include_trees includes = darray_new();

    // TODO: check duplicate sections
    // TODO: check required sections

    /* Collect section files */
    for (file = (XkbFile *) file->defs; file;
         file = (XkbFile *) file->common.next)
    {
        if (file->file_type >= FIRST_KEYMAP_FILE_TYPE ||
            file->file_type <= LAST_KEYMAP_FILE_TYPE)
        {
            struct include_tree *subtree;
            subtree = xkb_get_component_include_tree(ctx, file);
            if (!subtree) {
                // TODO handle better the error?
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "ERROR: cannot process %s\n",
                        xkb_file_type_to_string(file->file_type));
            } else {
                darray_append(includes, *subtree);
                darray_init(subtree->included);
                free(subtree);
            }
        }
    }

    tree->included = includes;
    darray_init(includes);
    return tree;
}

struct include_tree *
xkb_get_include_tree_from_file_v1(struct xkb_context *ctx,
                                  char *file_name, char *map, FILE *file)
{
    XkbFile *xkb_file;

    xkb_file = XkbParseFile(ctx, file, file_name, map);
    if (!xkb_file) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Failed to parse input xkb file\n");
        return false;
    }

    if (xkb_file->file_type >= FIRST_KEYMAP_FILE_TYPE &&
        xkb_file->file_type <= LAST_KEYMAP_FILE_TYPE)
    {
        struct include_tree *tree = xkb_get_component_include_tree(ctx, xkb_file);
        FreeXkbFile(xkb_file);
        return tree;
    } else if (xkb_file->file_type == FILE_TYPE_KEYMAP) {
        struct include_tree *tree = xkb_get_keymap_include_tree(ctx, xkb_file);
        FreeXkbFile(xkb_file);
        return tree;
    } else {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Unsupported XKB file type\n");
        FreeXkbFile(xkb_file);
        return NULL;
    }
}

struct include_tree *
xkb_get_include_tree_from_names_v1(struct xkb_context *ctx,
                                   const struct xkb_rule_names *rmlvo)
{
    bool ok;
    struct xkb_component_names kccgst;

    log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID,
            "Compiling from RMLVO: rules '%s', model '%s', layout '%s', "
            "variant '%s', options '%s'\n",
            rmlvo->rules, rmlvo->model, rmlvo->layout, rmlvo->variant,
            rmlvo->options);

    ok = xkb_components_from_rules(ctx, rmlvo, &kccgst);

    if (!ok) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Couldn't look up rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                rmlvo->rules, rmlvo->model, rmlvo->layout, rmlvo->variant,
                rmlvo->options);
        return NULL;
    }

    log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID,
            "Compiling from KcCGST: keycodes '%s', types '%s', "
            "compat '%s', symbols '%s'\n",
            kccgst.keycodes, kccgst.types, kccgst.compat, kccgst.symbols);

    XkbFile *file = XkbFileFromComponents(ctx, &kccgst);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compat);
    free(kccgst.symbols);

    if (!file) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Failed to generate parsed XKB file from components\n");
        return NULL;
    }

    struct include_tree *tree = xkb_get_keymap_include_tree(ctx, file);
    FreeXkbFile(file);
    return tree;
}
