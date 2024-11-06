/*
 * Copyright © 2018 Red Hat, Inc.
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

// #define ENABLE_KEYMAP_SOCKET

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef ENABLE_KEYMAP_SOCKET
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include "xkbcommon/xkbcommon.h"
#if ENABLE_PRIVATE_APIS
#include "xkbcomp/xkbcomp-priv.h"
#include "xkbcomp/rules.h"
#endif
#ifdef ENABLE_KEYMAP_CACHE
#include "src/context.h"
#include "src/xkbcomp/cache.h"
#endif
#include "tools-common.h"
#include "src/utils.h"

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"

static bool verbose = false;
static enum output_format {
    FORMAT_RMLVO,
    FORMAT_KEYMAP,
    FORMAT_KCCGST,
    FORMAT_KEYMAP_FROM_XKB,
} output_format = FORMAT_KEYMAP;
static const char *includes[64];
static size_t num_includes = 0;

#ifdef ENABLE_KEYMAP_SOCKET

static void
usage(char **argv)
{
    printf("Usage: %s [OPTIONS]\n"
           "\n"
           "Start a server to compile keymaps\n"
           "Options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
           " --socket <path>\n"
           "    Path of the Unix socket\n"
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
           argv[0]);
}

#else

static void
usage(char **argv)
{
    printf("Usage: %s [OPTIONS]\n"
           "\n"
           "Compile the given RMLVO to a keymap and print it\n"
           "\n"
           "Options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
#if ENABLE_PRIVATE_APIS
           " --kccgst\n"
           "    Print a keymap which only includes the KcCGST component names instead of the full keymap\n"
#endif
           " --rmlvo\n"
           "    Print the full RMLVO with the defaults filled in for missing elements\n"
           " --from-xkb\n"
           "    Load the XKB file from stdin, ignore RMLVO options.\n"
#if ENABLE_PRIVATE_APIS
           "    This option must not be used with --kccgst.\n"
#endif
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
           argv[0], DEFAULT_XKB_RULES,
           DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
           DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
           DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

#endif

static bool
parse_options(int argc, char **argv, struct xkb_rule_names *names,
              char **socket_address)
{
    enum options {
        OPT_VERBOSE,
        OPT_KCCGST,
        OPT_RMLVO,
        OPT_FROM_XKB,
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
        OPT_SOCKET,
    };
    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"verbose",          no_argument,            0, OPT_VERBOSE},
#ifdef ENABLE_KEYMAP_SOCKET
        {"socket",           required_argument,      0, OPT_SOCKET},
#endif
#if ENABLE_PRIVATE_APIS
        {"kccgst",           no_argument,            0, OPT_KCCGST},
#endif
#ifndef ENABLE_KEYMAP_SOCKET
        {"rmlvo",            no_argument,            0, OPT_RMLVO},
        {"from-xkb",         no_argument,            0, OPT_FROM_XKB},
#endif
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
#ifndef ENABLE_KEYMAP_SOCKET
        {"rules",            required_argument,      0, OPT_RULES},
        {"model",            required_argument,      0, OPT_MODEL},
        {"layout",           required_argument,      0, OPT_LAYOUT},
        {"variant",          required_argument,      0, OPT_VARIANT},
        {"options",          required_argument,      0, OPT_OPTION},
#endif
        {0, 0, 0, 0},
    };

    while (1) {
        int c;
        int option_index = 0;
        c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv);
            exit(0);
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_SOCKET:
            *socket_address = optarg;
            break;
        case OPT_KCCGST:
            output_format = FORMAT_KCCGST;
            break;
        case OPT_RMLVO:
            output_format = FORMAT_RMLVO;
            break;
        case OPT_FROM_XKB:
            output_format = FORMAT_KEYMAP_FROM_XKB;
            break;
        case OPT_INCLUDE:
            if (num_includes >= ARRAY_SIZE(includes)) {
                fprintf(stderr, "error: too many includes\n");
                exit(EXIT_INVALID_USAGE);
            }
            includes[num_includes++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (num_includes >= ARRAY_SIZE(includes)) {
                fprintf(stderr, "error: too many includes\n");
                exit(EXIT_INVALID_USAGE);
            }
            includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
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
            usage(argv);
            exit(EXIT_INVALID_USAGE);
        }

    }

    return true;
}

#ifndef ENABLE_KEYMAP_SOCKET
static bool
print_rmlvo(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
    printf("rules: \"%s\"\nmodel: \"%s\"\nlayout: \"%s\"\nvariant: \"%s\"\noptions: \"%s\"\n",
           rmlvo->rules, rmlvo->model, rmlvo->layout,
           rmlvo->variant ? rmlvo->variant : "",
           rmlvo->options ? rmlvo->options : "");
    return true;
}

static bool
print_kccgst(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
#if ENABLE_PRIVATE_APIS
        struct xkb_component_names kccgst;

        if (!xkb_components_from_rules(ctx, rmlvo, &kccgst, NULL))
            return false;

        printf("xkb_keymap {\n"
               "  xkb_keycodes { include \"%s\" };\n"
               "  xkb_types { include \"%s\" };\n"
               "  xkb_compat { include \"%s\" };\n"
               "  xkb_symbols { include \"%s\" };\n"
               "};\n",
               kccgst.keycodes, kccgst.types, kccgst.compat, kccgst.symbols);
        free(kccgst.keycodes);
        free(kccgst.types);
        free(kccgst.compat);
        free(kccgst.symbols);

        return true;
#else
        return false;
#endif
}

static bool
print_keymap(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new_from_names(ctx, rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap == NULL)
        return false;

    char *buf = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    printf("%s\n", buf);
    free(buf);

    xkb_keymap_unref(keymap);
    return true;
}

static bool
print_keymap_from_file(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap = NULL;
    char *keymap_string = NULL;
    FILE *file = NULL;
    bool success = false;

    file = tmpfile();
    if (!file) {
        fprintf(stderr, "Failed to create tmpfile\n");
        goto out;
    }

    while (true) {
        char buf[4096];
        size_t len;

        len = fread(buf, 1, sizeof(buf), stdin);
        if (ferror(stdin)) {
            fprintf(stderr, "Failed to read from stdin\n");
            goto out;
        }
        if (len > 0) {
            size_t wlen = fwrite(buf, 1, len, file);
            if (wlen != len) {
                fprintf(stderr, "Failed to write to tmpfile\n");
                goto out;
            }
        }
        if (feof(stdin))
            break;
    }
    fseek(file, 0, SEEK_SET);
    keymap = xkb_keymap_new_from_file(ctx, file,
                                      XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    if (!keymap) {
        fprintf(stderr, "Couldn't create xkb keymap\n");
        goto out;
    }

    keymap_string = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!keymap_string) {
        fprintf(stderr, "Couldn't get the keymap string\n");
        goto out;
    }

    fputs(keymap_string, stdout);
    success = true;

out:
    if (file)
        fclose(file);
    xkb_keymap_unref(keymap);
    free(keymap_string);

    return success;
}

#else

#define INPUT_BUFFER_SIZE 1024
static bool
parse_component(char **input, size_t *max_len, const char **output)
{
    if (!(*input) || !(*max_len)) {
        *input = NULL;
        *max_len = 0;
        *output = NULL;
        return true;
    }
    char *start = *input;
    char *next = strchr(start, '\n');
    size_t len;
    if (next == NULL) {
        len = *max_len;
        *input = NULL;
        *max_len = 0;
    } else {
        len = (size_t)(next - start);
        *max_len -= len + 1;
        *input += len + 1;
    }
    *output = strndup(start, len);
    if (!(*output)) {
        fprintf(stderr, "ERROR: Cannot allocate memory\n");
        return false;
    }
    return true;
}

static int
process_query(struct xkb_context *ctx, int accept_socket_fd)
{
    char input_buffer[INPUT_BUFFER_SIZE];
    ssize_t count = read(accept_socket_fd, input_buffer, INPUT_BUFFER_SIZE);
    if (count <= 0) {
        // TODO: error message
        return EXIT_FAILURE;
    }
    if (input_buffer[0] == '\x1b') {
        // Escape = exit
        return -1;
    }
    /* We expect RMLVO to be provided with one component per line */
    char *input = input_buffer;
    size_t len = count;
    struct xkb_rule_names rmlvo = {
        .rules = NULL,
        .model = NULL,
        .layout = NULL,
        .variant = NULL,
        .options = NULL,
    };
    int rc = EXIT_FAILURE;
    if (!parse_component(&input, &len, &rmlvo.rules) ||
        !parse_component(&input, &len, &rmlvo.model) ||
        !parse_component(&input, &len, &rmlvo.layout) ||
        !parse_component(&input, &len, &rmlvo.variant) ||
        !parse_component(&input, &len, &rmlvo.options)) {
        fprintf(stderr, "ERROR: Cannor parse RMLVO: %s.\n", input_buffer);
        goto error;
    }

    /* Compile keymap */
    struct xkb_keymap *keymap;
    keymap = xkb_keymap_new_from_names(ctx, &rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap == NULL)
        goto error;

    char *buf = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    write(accept_socket_fd, buf, strlen(buf) + 1);
    free(buf);

    xkb_keymap_unref(keymap);
    rc = EXIT_SUCCESS;

error:
    free((char*)rmlvo.rules);
    free((char*)rmlvo.model);
    free((char*)rmlvo.layout);
    free((char*)rmlvo.variant);
    free((char*)rmlvo.options);
    return rc;
}

static int
serve(struct xkb_context *ctx, const char* socket_address)
{
    int rc = EXIT_FAILURE;
    struct sockaddr_un sockaddr_un = { 0 };
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "ERROR: Cannot create Unix socket.\n");
        return EXIT_FAILURE;
    }
    /* Construct the bind address structure. */
    sockaddr_un.sun_family = AF_UNIX;
    strcpy(sockaddr_un.sun_path, socket_address);

    rc = bind(socket_fd, (struct sockaddr*) &sockaddr_un,
              sizeof(struct sockaddr_un));
    /* If socket_address exists on the filesystem, then bind will fail. */
    if (rc == -1) {
        fprintf(stderr, "ERROR: Cannot create Unix socket path.\n");
        rc = EXIT_FAILURE;
        goto error_bind;
    };
    if (listen(socket_fd, 4096) == -1) {
        fprintf(stderr, "ERROR: Cannot listen socket.\n");
        goto error;
    }

    while (1) {
        int accept_socket_fd = accept(socket_fd, NULL, NULL);
        if (accept_socket_fd == -1) {
            // TODO
            rc = EXIT_FAILURE;
            goto error;
        };

        if (accept_socket_fd > 0) {
            /* client is connected */
            rc = process_query(ctx, accept_socket_fd);
            close(accept_socket_fd);
            if (rc > 0) {
                fprintf(stderr, "ERROR: failed to process query\n");
            } else if (rc < 0) {
                fprintf(stderr, "Exiting...\n");
                break;
            }
        }
    }
error:
    close(socket_fd);
    unlink(socket_address);
error_bind:
    return rc;
}
#endif

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    struct xkb_rule_names names = {
        .rules = DEFAULT_XKB_RULES,
        .model = DEFAULT_XKB_MODEL,
        /* layout and variant are tied together, so we either get user-supplied for
         * both or default for both, see below */
        .layout = NULL,
        .variant = NULL,
        .options = DEFAULT_XKB_OPTIONS,
    };
    char *socket_address = NULL;
    int rc = EXIT_FAILURE;

    if (argc < 1) {
        usage(argv);
        return EXIT_INVALID_USAGE;
    }

    if (!parse_options(argc, argv, &names, &socket_address))
        return EXIT_INVALID_USAGE;

#ifndef ENABLE_KEYMAP_SOCKET
    /* Now fill in the layout */
    if (!names.layout || !*names.layout) {
        if (names.variant && *names.variant) {
            fprintf(stderr, "Error: a variant requires a layout\n");
            return EXIT_INVALID_USAGE;
        }
        names.layout = DEFAULT_XKB_LAYOUT;
        names.variant = DEFAULT_XKB_VARIANT;
    }
#endif

    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    assert(ctx);

    if (verbose) {
        xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_DEBUG);
        xkb_context_set_log_verbosity(ctx, 10);
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

#ifdef ENABLE_KEYMAP_SOCKET
#ifdef ENABLE_KEYMAP_CACHE
    struct xkb_keymap_cache *keymap_cache = xkb_keymap_cache_new();
    if (keymap_cache)
        ctx->keymap_cache = keymap_cache;
    else
        fprintf(stderr, "ERROR: Cannot create keymap cache.\n");
#endif
    serve(ctx, socket_address);
#ifdef ENABLE_KEYMAP_CACHE
    xkb_keymap_cache_free(keymap_cache);
#endif
#else
    if (output_format == FORMAT_RMLVO) {
        rc = print_rmlvo(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (output_format == FORMAT_KEYMAP) {
        rc = print_keymap(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (output_format == FORMAT_KCCGST) {
        rc = print_kccgst(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (output_format == FORMAT_KEYMAP_FROM_XKB) {
        rc = print_keymap_from_file(ctx);
    }
#endif

    xkb_context_unref(ctx);

    return rc;
}
