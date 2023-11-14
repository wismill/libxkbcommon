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
#include <dirent.h>
#include <sys/stat.h>

#include "darray.h"
#include "src/xkbcomp/include-list-priv.h"

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"
#define MAX_INCLUDES 64

static void
usage(FILE *fp, char *progname)
{
    fprintf(fp,
            "Usage: %s [OPTIONS] ... rules\n"
            "\n"
            "Print the includes of XKB files in YAML format\n"
            "\n"
            "Options:\n"
            " --help\n"
            "    Print this help and exit\n"
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
            "\n",
            progname);
}

struct xkb_file_ref {
    char *path;
    char *map;
};

typedef darray (char*) darray_string;

static bool
parse_options(int argc, char **argv, const char *(*includes)[MAX_INCLUDES],
              size_t *num_includes, darray_string *rules)
{
    enum options {
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
    };
    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {0, 0, 0, 0},
    };

    int option_index = 0;
    while (1) {
        int opt;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case 'h':
            usage(stdout, argv[0]);
            exit(EXIT_SUCCESS);
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
        default:
            usage(stderr, argv[0]);
            exit(EXIT_INVALID_USAGE);
        }
    }

    /* Add rules */
    for (; optind < argc; optind++) {
        darray_append(*rules, argv[optind]);
    }

    return true;
}

// #define INDENT_SIZE 2

// static void
// print_tree(struct xkb_context *ctx, struct include_tree *tree, unsigned int indent_size)
// {
//     struct include_tree *subtree;
//     const char *merge;
//     bool has_no_file;

//     if (tree->type != FILE_TYPE_KEYMAP) {

//         if (!indent_size) {
//             printf("- %s:\n", xkb_file_type_to_string(tree->type));
//             indent_size += INDENT_SIZE;
//         }

//         if (tree->file.file == XKB_ATOM_NONE) {
//             has_no_file = true;
//         } else {
//             has_no_file = false;
//             printf("%*s- file: \"%s(%s)\"\n", indent_size, "",
//                 xkb_atom_text(ctx, tree->file.file),
//                 xkb_atom_text(ctx, tree->file.map));
//         }

//         switch (tree->merge) {
//             case MERGE_DEFAULT:
//                 merge = "include";
//                 break;
//             case MERGE_AUGMENT:
//                 merge = "augment";
//                 break;
//             case MERGE_OVERRIDE:
//                 merge = "override";
//                 break;
//             case MERGE_REPLACE:
//                 merge = "replace";
//                 break;
//             default:
//                 merge = "(unknown)";
//         }
//         printf("%*s%smerge mode: %s\n", indent_size, "",
//                has_no_file ? "- " : "  ", merge);
//         if (darray_size(tree->included)) {
//             printf("%*s  included files:\n", indent_size, "");
//             darray_foreach(subtree, tree->included) {
//                 print_tree(ctx, subtree, indent_size + INDENT_SIZE);
//             }
//         }
//     } else {
//         if (darray_size(tree->included)) {
//             darray_foreach(subtree, tree->included) {
//                 print_tree(ctx, subtree, indent_size);
//             }
//         }
//     }
// }

static bool
analyze_file(struct xkb_context *ctx, const char *path, char *file_name)
{
    /* Load file */
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror(path);
        return false;
    }

    // bool ok;
    // char *string;
    // size_t size;

    // ok = map_file(file, &string, &size);
    // if (!ok) {
    //     log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
    //             "Couldn't read XKB file %s: %s\n",
    //             file_name, strerror(errno));
    //     return false;
    // }

    includes_atoms sections = darray_new();
    xkb_file_get_sections_names_from_file_v1(
        ctx, path, file, &sections
    );

    fprintf(stderr, "Found %d sections\n", darray_size(sections));

    struct include_atom *atom;
    darray_foreach(atom, sections) {
        printf("- %s(%s)%s\n",
            xkb_atom_text(ctx, atom->file),
            xkb_atom_text(ctx, atom->map),
            atom->is_map_default ? " (default)" : "");
    }
    printf("\n");
    darray_free(sections);

    // ok = xkb_file_get_sections_names_from_string_v1(ctx, string, size,
    //                                                 file_name, sections);
    // unmap_file(string, size);
    fclose(file);


    // return ok;

    // tree = xkb_get_include_tree_from_file_v1(ctx, path, from_xkb.map, file);

    // if (!tree) {
    //     fprintf(stderr, "Couldn't load include tree of file: %s\n", from_xkb.path);
    //     ret = EXIT_FAILURE;
    //     goto out;
    // }

    return true;
}

static void
analyze_path(struct xkb_context *ctx, const char *path)
{
    DIR *midir;
    struct dirent* info;
    struct stat file_stat;

    fprintf(stderr, "~~~ Analyze path: %s ~~~\n", path);
    if ((midir=opendir(path)) == NULL) {
        perror("Error in opendir");
        // exit(-1);// FIXME
        return;
    }
    while ((info = readdir(midir)) != 0) {
        char *full_path = asprintf_safe("%s/%s", path, info->d_name);
        if (!stat(full_path, &file_stat)) {
            if (S_ISDIR(file_stat.st_mode)) {
                if (strcmp(info->d_name, ".") != 0 && strcmp(info->d_name, "..") != 0) {
                    analyze_path(ctx, full_path);
                }
            } else {
                fprintf(stderr, "Found file: %s\n", info->d_name);
                analyze_file(ctx, full_path, info->d_name);
            }
        }
        free(full_path);
    }
    closedir(midir);
}

static void
check(struct xkb_context *ctx)
{
    for (unsigned int idx = 0; idx < xkb_context_num_include_paths(ctx); idx++) {
        const char *include_path = xkb_context_include_path_get(ctx, idx);
        fprintf(stderr, "=== Include path: %s ===\n", include_path);
        const char* components[4] = {"keycodes", "compat", "types", "symbols"};
        for (int k = 0; k < 4; k++) {
            char *path = asprintf_safe("%s/%s", include_path, components[k]);
            analyze_path(ctx, path);
            free(path);
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
    darray_string rule_names = darray_new();
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

    if (!parse_options(argc, argv, &includes, &num_includes, &rule_names))
        return EXIT_INVALID_USAGE;

    // /* Now fill in the layout */
    // if (!names.layout || !*names.layout) {
    //     if (names.variant && *names.variant) {
    //         fprintf(stderr, "Error: a variant requires a layout\n");
    //         return EXIT_INVALID_USAGE;
    //     }
    //     names.layout = DEFAULT_XKB_LAYOUT;
    //     names.variant = DEFAULT_XKB_VARIANT;
    // }

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

    char **rule;
    darray_foreach(rule, rule_names) {
        fprintf(stderr, "=== Rule: %s ===\n", *rule);
    }

    check(ctx);

    // if (from_xkb.path != NULL) {
    //     FILE *file = fopen(from_xkb.path, "rb");
    //     if (file == NULL) {
    //         perror(from_xkb.path);
    //         goto file_error;
    //     }

    //     tree = xkb_get_include_tree_from_file_v1(ctx, from_xkb.path,
    //                                              from_xkb.map, file);

    //     fclose(file);
    //     if (!tree) {
    //         fprintf(stderr, "Couldn't load include tree of file: %s\n", from_xkb.path);
    //         ret = EXIT_FAILURE;
    //         goto out;
    //     }
    // } else {
    //     tree = xkb_get_include_tree_from_names_v1(ctx, &names);
    // }
    // xkb_include_tree_subtree_free(tree);

    free(tree);

out:
    darray_free(rule_names);
file_error:
    xkb_context_unref(ctx);

    return ret;
}
