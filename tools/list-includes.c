/*
 * Copyright © 2023 Pierre Le Marre
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
 */

#include "config.h"

#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "darray.h"
#include "src/xkbcomp/include-list-priv.h"

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"
#define MAX_INCLUDES 64

static void
usage(FILE *fp, char *progname)
{
    fprintf(fp,
            "Usage: %s [OPTIONS]\n"
            "\n"
            "Print the includes of XKB files in YAML format\n"
            "\n"
            "Options:\n"
            " --help\n"
            "    Print this help and exit\n"
            " --file <path>\n"
            "    Load the XKB file <path>, ignore RMLVO options.\n"
            " --file-map <map>\n"
            "    When using --file, load a specific map.\n"
            " --include\n"
            "    Add the given path to the include path list. This option is\n"
            "    order-dependent, include paths given first are searched first.\n"
            "    If an include path is given, the default include path list is\n"
            "    not used. Use --include-defaults to add the default include\n"
            "    paths\n"
            " --include-defaults\n"
            "    Add the default set of include directories.\n"
            "    This option is order-dependent, include paths given first\n"
            "    are searched first.\n"
            "\n"
            "XKB-specific options:\n"
            " --rules <rules>\n"
            "    The XKB ruleset (default: '%s')\n"
            " --model <model>\n"
            "    The XKB model (default: '%s')\n"
            " --layout <layout>\n"
            "    The XKB layout (default: '%s')\n"
            " --variant <variant>\n"
            "    The XKB layout variant (default: '%s')\n"
            " --options <options>\n"
            "    The XKB options (default: '%s')\n"
            "\n",
            progname,
            DEFAULT_XKB_RULES,
            DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
            DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
            DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

struct xkb_file_ref {
    char *path;
    char *map;
};

static bool
parse_options(int argc, char **argv, const char *(*includes)[MAX_INCLUDES],
              size_t *num_includes, struct xkb_file_ref *from_xkb,
              struct xkb_rule_names *names)
{
    enum options {
        OPT_FROM_XKB,
        OPT_FROM_XKB_MAP,
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
    };
    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"file",             required_argument,      0, OPT_FROM_XKB},
        {"file-map",         required_argument,      0, OPT_FROM_XKB_MAP},
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {"rules",            required_argument,      0, OPT_RULES},
        {"model",            required_argument,      0, OPT_MODEL},
        {"layout",           required_argument,      0, OPT_LAYOUT},
        {"variant",          required_argument,      0, OPT_VARIANT},
        {"options",          required_argument,      0, OPT_OPTION},
        {0, 0, 0, 0},
    };

    from_xkb->path = NULL;
    from_xkb->map = NULL;

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case 'h':
            usage(stdout, argv[0]);
            exit(EXIT_SUCCESS);
        case OPT_FROM_XKB:
            from_xkb->path = optarg;
            break;
        case OPT_FROM_XKB_MAP:
            from_xkb->map = optarg;
            break;
        case OPT_INCLUDE:
            if (*num_includes >= ARRAY_SIZE(*includes)) {
                fprintf(stderr, "error: too many includes: got %ld, expected max %ld\n",
                        *num_includes, ARRAY_SIZE(*includes));
                exit(EXIT_INVALID_USAGE);
            }
            (*includes)[(*num_includes)++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (*num_includes >= ARRAY_SIZE(*includes)) {
                fprintf(stderr, "error: too many includes: got %ld, expected max %ld\n",
                        *num_includes, ARRAY_SIZE(*includes));
                exit(EXIT_INVALID_USAGE);
            }
            (*includes)[(*num_includes)++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
            break;
        case OPT_RULES:
            names->rules = optarg;
            break;
        case OPT_MODEL:
            names->model = optarg;
            break;
        case OPT_LAYOUT:
            names->layout = optarg;
            break;
        case OPT_VARIANT:
            names->variant = optarg;
            break;
        case OPT_OPTION:
            names->options = optarg;
            break;
        default:
            usage(stderr, argv[0]);
            exit(EXIT_INVALID_USAGE);
        }
    }

    return true;
}

#define INDENT_SIZE 2

static void
print_tree(struct xkb_context *ctx, struct include_tree *tree, unsigned int indent_size)
{
    struct include_tree *subtree;
    const char *merge;
    bool has_no_file;

    if (tree->type != FILE_TYPE_KEYMAP) {

        if (!indent_size) {
            printf("- %s:\n", xkb_file_type_to_string(tree->type));
            indent_size += INDENT_SIZE;
        }

        if (tree->file.file == XKB_ATOM_NONE) {
            has_no_file = true;
        } else {
            has_no_file = false;
            printf("%*s- file: \"%s(%s)\"\n", indent_size, "",
                xkb_atom_text(ctx, tree->file.file),
                xkb_atom_text(ctx, tree->file.map));
        }

        switch (tree->merge) {
            case MERGE_DEFAULT:
                merge = "include";
                break;
            case MERGE_AUGMENT:
                merge = "augment";
                break;
            case MERGE_OVERRIDE:
                merge = "override";
                break;
            case MERGE_REPLACE:
                merge = "replace";
                break;
            default:
                merge = "(unknown)";
        }
        printf("%*s%smerge mode: %s\n", indent_size, "",
               has_no_file ? "- " : "  ", merge);
        if (darray_size(tree->included)) {
            printf("%*s  included files:\n", indent_size, "");
            darray_foreach(subtree, tree->included) {
                print_tree(ctx, subtree, indent_size + INDENT_SIZE);
            }
        }
    } else {
        if (darray_size(tree->included)) {
            darray_foreach(subtree, tree->included) {
                print_tree(ctx, subtree, indent_size);
            }
        }
    }
}

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct xkb_context *ctx = NULL;
    struct include_tree *tree = NULL;
    const char *includes[MAX_INCLUDES];
    size_t num_includes = 0;
    struct xkb_file_ref from_xkb;
    struct xkb_rule_names names = {
        .rules = DEFAULT_XKB_RULES,
        .model = DEFAULT_XKB_MODEL,
        /* layout and variant are tied together, so we either get user-supplied for
         * both or default for both, see below */
        .layout = NULL,
        .variant = NULL,
        .options = DEFAULT_XKB_OPTIONS,
    };

    if (argc < 1) {
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    if (!parse_options(argc, argv, &includes, &num_includes, &from_xkb, &names))
        return EXIT_INVALID_USAGE;

    /* Now fill in the layout */
    if (!names.layout || !*names.layout) {
        if (names.variant && *names.variant) {
            fprintf(stderr, "Error: a variant requires a layout\n");
            return EXIT_INVALID_USAGE;
        }
        names.layout = DEFAULT_XKB_LAYOUT;
        names.variant = DEFAULT_XKB_VARIANT;
    }

    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    if (!ctx) {
        fprintf(stderr, "Couldn't create xkb context\n");
        goto out;
    }

    if (num_includes == 0)
        includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < num_includes; i++) {
        const char *include = includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
            xkb_context_include_path_append_default(ctx);
        else
            xkb_context_include_path_append(ctx, include);
    }

    if (from_xkb.path != NULL) {
        FILE *file = fopen(from_xkb.path, "rb");
        if (file == NULL) {
            perror(from_xkb.path);
            goto file_error;
        }

        tree = xkb_get_include_tree_from_file_v1(ctx, from_xkb.path,
                                                 from_xkb.map, file);

        fclose(file);
        if (!tree) {
            fprintf(stderr, "Couldn't load include tree of file: %s\n", from_xkb.path);
            ret = EXIT_FAILURE;
            goto out;
        }
    } else {
        tree = xkb_get_include_tree_from_names_v1(ctx, &names);
    }

    print_tree(ctx, tree, 0);

    xkb_include_tree_subtree_free(tree);
    free(tree);

out:
file_error:
    xkb_context_unref(ctx);

    return ret;
}
