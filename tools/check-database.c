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
// #include <libgen.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbregistry.h"
#include "darray.h"
#include "src/atom.h"
#include "src/xkbcomp/ast.h"
#include "src/xkbcomp/ast-build.h"
#include "src/xkbcomp/include-list-priv.h"
#include "src/xkbcomp/xkbcomp-priv.h"
#include "src/xkbcomp/rules.h"

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

struct rulesets {
    xkb_atom_t rules;
    xkb_atom_t model;
    xkb_atom_t layout;
    xkb_atom_t variant;
    xkb_atom_t options;
};

struct registry_entry {
    struct rulesets rlmvo;
    bool root_include;
};

struct rmlvo_conf {
    xkb_atom_t rules;
    xkb_atom_t model;
    xkb_atom_t layout;
    xkb_atom_t variant;
    xkb_atom_t options;
};

struct registry_match {
    xkb_atom_t file;
    xkb_atom_t section;
    darray(struct rmlvo_conf) entries;
};

static void
registry_match_free(struct registry_match *m)
{
    darray_free(m->entries);
}

typedef darray(struct registry_match) registry_match_array;

struct registry_matches {
    registry_match_array keycodes;
    registry_match_array compat;
    registry_match_array symbols;
    registry_match_array types;
};

static void
registry_matches_free(struct registry_matches *matches)
{
    struct registry_match *m;
    darray_foreach(m, matches->keycodes) {
        registry_match_free(m);
    }
    darray_foreach(m, matches->compat) {
        registry_match_free(m);
    }
    darray_foreach(m, matches->symbols) {
        registry_match_free(m);
    }
    darray_foreach(m, matches->types) {
        registry_match_free(m);
    }
}

static struct registry_match *
registry_match_lookup(registry_match_array *array, xkb_atom_t file, xkb_atom_t section)
{
    struct registry_match *m;
    darray_foreach(m, *array) {
        if (m->file == file && m->section == section) {
            return m;
        }
    }
    /* Not found */
    return NULL;
}

static void
registry_match_add_entry(struct xkb_context *ctx, registry_match_array *array,
                         xkb_atom_t file, xkb_atom_t section,
                         struct xkb_rule_names *names)
{
    struct registry_match *m;
    if (!(m = registry_match_lookup(array, file, section))) {
        struct registry_match new = {
            .file = file,
            .section = section,
            .entries = darray_new()
        };
        size_t len = darray_size(*array);
        darray_append(*array, new);
        m = &darray_item(*array, len);
    }
    struct rmlvo_conf rmlvo = {
        .rules = xkb_atom_intern(ctx, names->rules, strlen(names->rules)),
        .model = xkb_atom_intern(ctx, names->model, strlen(names->model)),
        .layout = xkb_atom_intern(ctx, names->layout, strlen(names->layout)),
        .variant = names->variant
            ? xkb_atom_intern(ctx, names->variant, strlen(names->variant))
            : XKB_ATOM_NONE,
        .options = names->options
            ? xkb_atom_intern(ctx, names->options, strlen(names->options))
            : XKB_ATOM_NONE
    };
    darray_append(m->entries, rmlvo);
}

static void
registry_match_add_raw(struct xkb_context *ctx, registry_match_array *array,
                       char *include,
                       struct xkb_rule_names *names)
{
    IncludeStmt *inc = IncludeCreate(ctx, include, MERGE_DEFAULT);
    for (IncludeStmt *stmt = inc; stmt; stmt = stmt->next_incl) {
        xkb_atom_t file = xkb_atom_intern(ctx, stmt->file, strlen(stmt->file));
        xkb_atom_t section = stmt->map
            ? xkb_atom_intern(ctx, stmt->map, strlen(stmt->map))
            : XKB_ATOM_NONE;
        registry_match_add_entry(ctx, array, file, section, names);
    }
    FreeStmt((ParseCommon *) inc);
}

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
    registry_match_array *registry;
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

#define INDENT_LENGTH 2

static void
print_rmlvo(struct xkb_context *ctx, struct rmlvo_conf *rmlvo, unsigned int indent)
{
    printf("%*s- rules: \"%s\"\n", INDENT_LENGTH * indent, "", xkb_atom_text(ctx, rmlvo->rules));
    printf("%*s  model: \"%s\"\n", INDENT_LENGTH * indent, "", xkb_atom_text(ctx, rmlvo->model));
    printf("%*s  layout: \"%s\"\n", INDENT_LENGTH * indent, "", xkb_atom_text(ctx, rmlvo->layout));
    printf("%*s  variant: ", INDENT_LENGTH * indent, "");
    if (rmlvo->variant)
        printf("\"%s\"\n", xkb_atom_text(ctx, rmlvo->variant));
    else
        printf("null\n");
    printf("%*s  options: ", INDENT_LENGTH * indent, "");
    if (rmlvo->options)
        printf("\"%s\"\n", xkb_atom_text(ctx, rmlvo->options));
    else
        printf("null\n");
}

static bool
analyze_file(struct xkb_context *ctx, struct keymap_component *component,
             const char *path, char *file_name, unsigned int indent)
{
    printf("%*s\"%s\":\n", INDENT_LENGTH * indent, "", path);
    indent++;
    printf("%*sname: \"%s\"\n", INDENT_LENGTH * indent, "", file_name);

    /* Load file */
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror(path);
        printf("%*s  error: Cannot open file\n", INDENT_LENGTH * indent, "");
        return false;
    }

    /* Map file */
    bool ok;
    char *string;
    size_t size;

    ok = map_file(file, &string, &size);
    fclose(file);
    if (!ok) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Couldn't read XKB file %s: %s\n",
                file_name, strerror(errno));
        printf("%*serror: Cannot map file\n", INDENT_LENGTH * indent, "");
        return false;
    }

    unsigned int offset = 0;
    char *path2;
    file = FindFileInXkbPath(ctx, file_name, component->type, &path2, &offset);
    if (file)
        fclose(file);
    bool canonical = streq_not_null(path, path2);
    printf("%*scanonical: %s\n", INDENT_LENGTH * indent, "",
           canonical ? "true" : "false");
    if (!canonical && path2) {
        printf("%*soverriden by: \"%s\"\n", INDENT_LENGTH * indent, "", path2);
    }
    free(path2);

    struct include_atom atom = {
        .path = xkb_atom_intern(ctx, path, strlen(path)),
        .file = xkb_atom_intern(ctx, file_name, strlen(file_name)),
        .map = XKB_ATOM_NONE,
        .is_map_default = false,
        .valid = true
    };

    struct xkb_file_section_iterator *iter;
    iter = xkb_parse_iterator_new_from_string_v1(ctx, string, size, file_name);
    bool has_sections = false;


    struct xkb_file_section *section;
    while ((section = xkb_parse_iterator_next(iter, &ok))) {
        if (!has_sections) {
            has_sections = true;
            printf("%*ssections:\n", INDENT_LENGTH * indent, "");
        }
        atom.map = section->name;
        atom.is_map_default = section->is_default;
        printf("%*s- name: \"%s\"\n", INDENT_LENGTH * indent, "",
               xkb_atom_text(ctx, atom.map));
        if (atom.is_map_default) {
            printf("%*s  default: true\n", INDENT_LENGTH * indent, "");
        }
        if (canonical) {
            struct registry_match *m = registry_match_lookup(
                component->registry, atom.file, atom.map);
            if (!m && atom.is_map_default)
                m = registry_match_lookup(component->registry, atom.file, XKB_ATOM_NONE);
            if (m) {
                printf("%*s  registry:\n", INDENT_LENGTH * indent, "");
                indent++;
                struct rmlvo_conf *rmlvo;
                darray_foreach(rmlvo, m->entries) {
                    print_rmlvo(ctx, rmlvo, indent);
                }
                indent--;
            } else {
                printf("%*s  registry: null\n", INDENT_LENGTH * indent, "");
            }
        } else {
            printf("%*s  registry: []\n", INDENT_LENGTH * indent, "");
        }
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
        struct include_atom *inc;
        printf("%*s  includes:", INDENT_LENGTH * indent, "");
        if (!darray_empty(cur->includes)) {
            printf("\n");
            indent++;
            const char *section_name;
            darray_foreach(inc, cur->includes) {
                printf("%*s- name: \"%s\"\n", INDENT_LENGTH * indent, "",
                       xkb_atom_text(ctx, inc->file));
                printf("%*s  section: ", INDENT_LENGTH * indent, "");
                if ((section_name = xkb_atom_text(ctx, inc->map)))
                    printf("\"%s\"\n", section_name);
                else
                    printf("null\n");
                printf("%*s  path: \"%s\"\n", INDENT_LENGTH * indent, "",
                       xkb_atom_text(ctx, inc->path));
            }
            indent--;
        } else {
            printf(" []\n");
        }
        xkb_file_section_free(section);
    }

    xkb_parse_iterator_free(iter);

    if (!has_sections) {
        printf("%*serror: Parse error\n", INDENT_LENGTH * indent, "");
    }

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
             const char *path, unsigned int indent)
{
    // printf("%*s# path: %s\n", INDENT_LENGTH * indent, "", path);

    char *paths[] = { (char*) path, NULL};

    FTS *fts;
    if (!(fts = fts_open(paths, FTS_LOGICAL, NULL))) {
        // FIXME handle error
        return;
    }

    FTSENT *p;
    char relative_path[PATH_MAX];
    while ((p = fts_read(fts))) {
        switch (p->fts_info) {
            case FTS_DP:
                // FIXME directory
                break;
            case FTS_F:
                /* Relative path to */
                FTSENT *p2 = p->fts_parent;
                p2->fts_pointer = p;
                while (p2->fts_level > 0) {
                    p2->fts_parent->fts_pointer = p2;
                    p2 = p2->fts_parent;
                }
                size_t len = 0;
                while ((p2 = p2->fts_pointer)) {
                    strcpy(&relative_path[len], p2->fts_name);
                    len += p2->fts_namelen;
                    relative_path[len] = '/';
                    len++;
                }
                relative_path[len - 1] = '\0';
                analyze_file(ctx, component, p->fts_path, relative_path, indent);
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
check_files(struct xkb_context *ctx, struct registry_matches *registry)
{
    struct keymap_components components = {
        .keycodes = {
            .type = FILE_TYPE_KEYCODES,
            .files = darray_new(),
            .refs = darray_new(),
            .registry = &registry->keycodes
        },
        .compat = {
            .type = FILE_TYPE_COMPAT,
            .files = darray_new(),
            .refs = darray_new(),
            .registry = &registry->compat
        },
        .symbols = {
            .type = FILE_TYPE_SYMBOLS,
            .files = darray_new(),
            .refs = darray_new(),
            .registry = &registry->symbols
        },
        .types = {
            .type = FILE_TYPE_TYPES,
            .files = darray_new(),
            .refs = darray_new(),
            .registry = &registry->types
        }
    };
    struct keymap_component *comps[COMPONENT_COUNT] = {
        &components.keycodes,
        &components.compat,
        &components.symbols,
        &components.types
    };

    /* Get all files */
    for (int k = 0; k < COMPONENT_COUNT; k++) {
        printf("%s:\n", xkb_file_type_to_string(comps[k]->type));
        for (unsigned int idx = 0; idx < xkb_context_num_include_paths(ctx); idx++) {
            const char *include_path = xkb_context_include_path_get(ctx, idx);
            printf("  # include path: %s\n", include_path);
            char *path = asprintf_safe(
                "%s/%s",
                include_path, xkb_file_type_include_dirs[comps[k]->type]
            );
            analyze_path(ctx, comps[k], path, 1);
            free(path);
        }
    }

    keymap_components_free(&components);
}

static void
print_registry_entry(struct xkb_context *ctx, struct rxkb_context *rctx,
                     struct xkb_rule_names *names, unsigned int indent)
{
    printf("%*s- rmlvo:\n", INDENT_LENGTH * indent, "");
    indent += 2;
    printf("%*smodel: \"%s\"\n", INDENT_LENGTH * indent, "", names->model);
    printf("%*slayout: \"%s\"\n", INDENT_LENGTH * indent, "", names->layout);
    printf("%*svariant: \"%s\"\n", INDENT_LENGTH * indent, "", names->variant ? names->variant : "");
    printf("%*soptions: \"%s\"\n", INDENT_LENGTH * indent, "", names->options ? names->options : "");
    indent -= 2;
    printf("%*s  kccgst:\n", INDENT_LENGTH * indent, "");
    struct xkb_component_names kccgst;
    if (!xkb_components_from_rules(ctx, names, &kccgst))
        // FIXME
        return;
    indent += 2;
    printf("%*skeycodes: \"%s\"\n", INDENT_LENGTH * indent, "", kccgst.keycodes);
    printf("%*stypes: \"%s\"\n", INDENT_LENGTH * indent, "", kccgst.types);
    printf("%*scompat: \"%s\"\n", INDENT_LENGTH * indent, "", kccgst.compat);
    printf("%*ssymbols: \"%s\"\n", INDENT_LENGTH * indent, "", kccgst.symbols);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compat);
    free(kccgst.symbols);
}

static void
check_registry(struct xkb_context *ctx,
               const char *(*includes)[MAX_INCLUDES], size_t num_includes,
               darray_string *rulesets)
{
    printf("registry:\n");
    char **ruleset;
    unsigned int indent = 1;
    darray_foreach(ruleset, *rulesets) {
        struct rxkb_context *rctx = rxkb_context_new(
            RXKB_CONTEXT_NO_DEFAULT_INCLUDES | RXKB_CONTEXT_LOAD_EXOTIC_RULES
        );
        if (!rctx)
            goto rcontext_error;

        for (size_t i = 0; i < num_includes; i++) {
            const char *include = *includes[i];
            if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
                rxkb_context_include_path_append_default(rctx);
            else
                rxkb_context_include_path_append(rctx, include);
        }
        if (!rxkb_context_parse(rctx, *ruleset)) {
            fprintf(stderr, "Failed to parse XKB descriptions for ruleset: %s.\n", *ruleset);
            goto rcontext_error;
        }
        printf("%*s\"%s\":\n", INDENT_LENGTH * indent, "", *ruleset);
        printf("%*s# layouts\n", INDENT_LENGTH * indent, "");
        struct rxkb_model *m;
        struct rxkb_layout *l;
        l = rxkb_layout_first(rctx);
        while (l) {
            m = rxkb_model_first(rctx);
            while (m) {
                struct xkb_rule_names names = {
                    .rules = *ruleset,
                    .model = rxkb_model_get_name(m),
                    .layout = rxkb_layout_get_name(l),
                    .variant = rxkb_layout_get_variant(l),
                    .options = DEFAULT_XKB_OPTIONS,
                };
                print_registry_entry(ctx, rctx, &names, indent);
                m = rxkb_model_next(m);
            }
            l = rxkb_layout_next(l);
        }
        // TODO
        printf("%*s# options\n", INDENT_LENGTH * indent, "");
        // struct rxkb_option_group *g;
        // TODO
rcontext_error:
        rxkb_context_unref(rctx);
    }
}

static void
load_registry(struct xkb_context *ctx,
              const char *(*includes)[MAX_INCLUDES], size_t num_includes,
              darray_string *rulesets, struct registry_matches *registry)
{
    char **ruleset;
    darray_foreach(ruleset, *rulesets) {
        struct rxkb_context *rctx = rxkb_context_new(
            RXKB_CONTEXT_NO_DEFAULT_INCLUDES | RXKB_CONTEXT_LOAD_EXOTIC_RULES
        );
        if (!rctx)
            goto rcontext_error;

        for (size_t i = 0; i < num_includes; i++) {
            const char *include = *includes[i];
            if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
                rxkb_context_include_path_append_default(rctx);
            else
                rxkb_context_include_path_append(rctx, include);
        }
        if (!rxkb_context_parse(rctx, *ruleset)) {
            fprintf(stderr, "Failed to parse XKB descriptions for ruleset: %s.\n", *ruleset);
            goto rcontext_error;
        }
        // struct rxkb_model *m;
        struct rxkb_layout *l;
        l = rxkb_layout_first(rctx);
        while (l) {
            // m = rxkb_model_first(rctx);
            // while (m) {
                struct xkb_rule_names names = {
                    .rules = *ruleset,
                    // .model = rxkb_model_get_name(m),
                    .model = DEFAULT_XKB_MODEL,
                    .layout = rxkb_layout_get_name(l),
                    .variant = rxkb_layout_get_variant(l),
                    .options = DEFAULT_XKB_OPTIONS,
                };
                // print_registry_entry(ctx, rctx, &names, indent);
                struct xkb_component_names kccgst;
                if (!xkb_components_from_rules(ctx, &names, &kccgst))
                    continue; // FIXME
                registry_match_add_raw(ctx, &registry->keycodes, kccgst.keycodes, &names);
                registry_match_add_raw(ctx, &registry->compat, kccgst.compat, &names);
                registry_match_add_raw(ctx, &registry->symbols, kccgst.symbols, &names);
                registry_match_add_raw(ctx, &registry->types, kccgst.types, &names);
                free(kccgst.keycodes);
                free(kccgst.types);
                free(kccgst.compat);
                free(kccgst.symbols);
                // m = rxkb_model_next(m);
            // }
            l = rxkb_layout_next(l);
        }
        // TODO
        // struct rxkb_option_group *g;
rcontext_error:
        rxkb_context_unref(rctx);
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
    darray_string rulesets = darray_new();
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

    if (!parse_options(argc, argv, &includes, &num_includes, &rulesets))
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
        goto context_error;
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

    struct registry_matches registry = {
        .keycodes = darray_new(),
        .compat = darray_new(),
        .symbols = darray_new(),
        .types = darray_new()
    };

    load_registry(ctx, &includes, num_includes, &rulesets, &registry);

    check_files(ctx, &registry);
    free(tree);

    registry_matches_free(&registry);

    // /* Registry listing */
    // check_registry(ctx, &includes, num_includes, &rulesets);

context_error:
    xkb_context_unref(ctx);

out:
    darray_free(rulesets);

    return ret;
}
