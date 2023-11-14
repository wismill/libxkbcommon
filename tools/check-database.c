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
#include <stdint.h>
#include <stdbool.h>
// #include <dirent.h>
// #include <sys/stat.h>
#include <fts.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbregistry.h"
#include "darray.h"
#include "src/atom.h"
#include "src/xkbcomp/ast.h"
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

struct rule_names {
    xkb_atom_t rules;
    xkb_atom_t model;
    xkb_atom_t layout;
    xkb_atom_t variant;
    xkb_atom_t options;
};

struct registry_entry {
    struct rule_names rlmvo;
    bool root_include;
};

struct keymap_component_file {
    struct include_atom file;
    darray(struct include_atom) includes;
    darray(struct include_atom) reverse_includes;
    darray(struct registry_entry) registry_entries;
};

struct keymap_component {
    enum xkb_file_type type;
    darray(struct keymap_component_file) files;
    darray(struct include_atom) refs;
};

struct keymap_components {
    struct keymap_component keycodes;
    struct keymap_component compat;
    struct keymap_component symbols;
    struct keymap_component types;
};

static void
keymap_component_free(struct keymap_component *component)
{
    struct keymap_component_file *file;
    darray_foreach(file, component->files) {
        darray_free(file->includes);
        darray_free(file->reverse_includes);
        darray_free(file->registry_entries);
    }
    darray_free(component->files);
    darray_free(component->refs);
}

static void
keymap_components_free(struct keymap_components *components)
{
    keymap_component_free(&components->keycodes);
    keymap_component_free(&components->compat);
    keymap_component_free(&components->symbols);
    keymap_component_free(&components->types);
}

static bool
analyze_file(struct xkb_context *ctx, struct keymap_component *component,
             const char *path, char *file_name)
{
    /* Load file */
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror(path);
        return false;
    }

    bool ok;
    char *string;
    size_t size;

    ok = map_file(file, &string, &size);
    fclose(file);
    if (!ok) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Couldn't read XKB file %s: %s\n",
                file_name, strerror(errno));
        return false;
    }

    struct include_atom atom = {
        .path = xkb_atom_intern(ctx, path, strlen(path)),
        .file = xkb_atom_intern(ctx, file_name, strlen(file_name)),
        .map = XKB_ATOM_NONE,
        .is_map_default = false,
        .valid = true
    };

    struct xkb_file_section_iterator *iter;
    iter = xkb_parse_iterator_new_from_string_v1(ctx, string, size, file_name);

    struct xkb_file_section *section;
    while ((section = xkb_parse_iterator_next(iter, &ok))) {
        atom.map = section->name;
        atom.is_map_default = section->is_default;
        struct keymap_component_file comp_file = {
            .file = atom,
            .includes = darray_new(),
            .reverse_includes = darray_new(),
            .registry_entries = darray_new()
        };
        size_t idx = darray_size(component->files);
        darray_append(component->files, comp_file);
        struct keymap_component_file *cur = &darray_item(component->files, idx);
        darray_copy(cur->includes, section->includes);
        xkb_file_section_free(section);
    }

    xkb_parse_iterator_free(iter);

    /*
    struct xkb_file_section_iterator *iter;
    iter = xkb_parse_iterator_new_from_string_v1(ctx, string, size, path);

    XkbFile *xkb_file;

    while ((xkb_file = xkb_parse_iterator_next_legacy(iter, &ok))) {
        // TODO: use "ok"
        struct include_atom atom;
        xkb_create_include_atom(ctx, xkb_file, &atom);
        // darray_append(*sections, atom);
        printf("- %s(%s)%s\n",
            xkb_atom_text(ctx, atom.file),
            xkb_atom_text(ctx, atom.map),
            atom.is_map_default ? " (default)" : "");
        struct include_tree *tree = xkb_get_component_include_tree(ctx, xkb_file);
        // TODO save tree
        xkb_include_tree_subtrees_free(tree);
        free(tree);
        // FIXME FreeXkbFile(file);
    }

    xkb_parse_iterator_free(iter);
    */

    unmap_file(string, size);

    return true;
}

static void
analyze_path(struct xkb_context *ctx, struct keymap_component *component,
             const char *path)
{
    fprintf(stderr, "~~~ Analyze path: %s ~~~\n", path);

    char *paths[] = { (char*) path, NULL};

    FTS *fts;
    if (!(fts = fts_open(paths, FTS_LOGICAL, NULL))) {
        // FIXME handle error
        return;
    }

    FTSENT *p;
    while ((p = fts_read(fts))) {
        switch (p->fts_info) {
            case FTS_DP:
                // FIXME directory
                break;
            case FTS_F:
                fprintf(stderr, "#### File: %s\n", p->fts_accpath);
                analyze_file(ctx, component, p->fts_accpath, p->fts_name);
                break;
            case FTS_SL:
            case FTS_SLNONE:
                // FIXME symbolic link
                // fprintf(stderr, "#### Symbolic link: %s\n", p->fts_accpath);
                break;
        }
    }
    // TODO check errno??

    fts_close(fts);

    return;

    // DIR *my_dir;
    // struct dirent* info;
    // struct stat file_stat;

    // if ((my_dir=opendir(path)) == NULL) {
    //     perror("Error in opendir");
    //     // exit(-1);// FIXME
    //     return;
    // }

    // // TODO check max opened files with readdir
    // while ((info = readdir(my_dir)) != 0) {
    //     char *full_path = asprintf_safe("%s/%s", path, info->d_name);
    //     if (!stat(full_path, &file_stat)) {
    //         if (S_ISDIR(file_stat.st_mode)) {
    //             if (strcmp(info->d_name, ".") != 0 && strcmp(info->d_name, "..") != 0) {
    //                 analyze_path(ctx, full_path);
    //             }
    //         } else {
    //             fprintf(stderr, "Processing file: %s\n", full_path);
    //             analyze_file(ctx, full_path, info->d_name);
    //         }
    //     }
    //     free(full_path);
    // }
    // closedir(my_dir);
}

// TODO: standard way to get directories names?
static const char *xkb_file_type_include_dirs[_FILE_TYPE_NUM_ENTRIES] = {
    [FILE_TYPE_KEYCODES] = "keycodes",
    [FILE_TYPE_TYPES] = "types",
    [FILE_TYPE_COMPAT] = "compat",
    [FILE_TYPE_SYMBOLS] = "symbols",
    [FILE_TYPE_GEOMETRY] = "geometry",
    [FILE_TYPE_KEYMAP] = "keymap",
    [FILE_TYPE_RULES] = "rules",
};

#define COMPONENT_COUNT 4

static void
check(struct xkb_context *ctx)
{
    struct keymap_components components = {
        .keycodes = {
            .type = FILE_TYPE_KEYCODES,
            .files = darray_new(),
            .refs = darray_new()
        },
        .compat = {
            .type = FILE_TYPE_COMPAT,
            .files = darray_new(),
            .refs = darray_new()
        },
        .symbols = {
            .type = FILE_TYPE_SYMBOLS,
            .files = darray_new(),
            .refs = darray_new()
        },
        .types = {
            .type = FILE_TYPE_TYPES,
            .files = darray_new(),
            .refs = darray_new()
        }
    };
    struct keymap_component *comps[COMPONENT_COUNT] = {
        &components.keycodes,
        &components.compat,
        &components.symbols,
        &components.types
    };

    /* Get all files */
    for (unsigned int idx = 0; idx < xkb_context_num_include_paths(ctx); idx++) {
        const char *include_path = xkb_context_include_path_get(ctx, idx);
        fprintf(stderr, "=== Include path: %s ===\n", include_path);
        for (int k = 0; k < COMPONENT_COUNT; k++) {
            char *path = asprintf_safe(
                "%s/%s",
                include_path, xkb_file_type_include_dirs[comps[k]->type]
            );
            analyze_path(ctx, comps[k], path);
            free(path);
        }
    }

    keymap_components_free(&components);
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
