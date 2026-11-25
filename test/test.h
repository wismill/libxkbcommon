/*
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */
#pragma once

#include "config.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* Don't use compat names in internal code. */
#define _XKBCOMMON_COMPAT_H
#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"
#include "src/messages-codes.h"
#include "utils.h"

/* Automake test exit code to signify SKIP (à la PASS, FAIL, etc).
 * See: https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html */
#define SKIP_TEST 77
#define TEST_SETUP_FAILURE 99
#define TEST_ASSERT_FAILURE -1

#define assert_printf(cond, ...) do {                      \
    const bool __cond = (cond);                            \
    if (!__cond) {                                         \
       fprintf(stderr, "Assertion failure: " __VA_ARGS__); \
       assert(__cond);                                     \
    }} while (0)

#define assert_streq(test_name, expected, got) \
    assert_printf(streq_null(expected, got), \
                  test_name ". Expected \"%s\", got: \"%s\"\n", expected, got)

#define assert_streq_not_null(test_name, expected, got) \
    assert_printf(streq_not_null(expected, got), \
                  test_name ". Expected \"%s\", got: \"%s\"\n", expected, got)

#define assert_eq(test_name, expected, got, format, ...) \
    assert_printf(expected == got, \
                  test_name ". Expected " format ", got: " format "\n", \
                  ##__VA_ARGS__, expected, got)

/* Enable testing assertion failures */
#if defined(__unix__) || defined(__APPLE__)
    #include <sys/wait.h>
    #include <sys/mman.h>
    #include <unistd.h>
    #define RUN_ISOLATED(code, ret, ...) do {           \
        memset(&(ret), 0, sizeof(ret));                 \
        __typeof__(ret) *_result = mmap(                \
            NULL, sizeof(ret),                          \
            PROT_READ | PROT_WRITE,                     \
            MAP_SHARED | MAP_ANONYMOUS, -1, 0           \
        );                                              \
        if (_result == MAP_FAILED) {                    \
            (code) = TEST_SETUP_FAILURE;                \
            break;                                      \
        }                                               \
        pid_t _pid = fork();                            \
        if (_pid == -1) {                               \
            (code) = TEST_SETUP_FAILURE;                \
            munmap(_result, sizeof(ret));               \
        } else if (_pid == 0) {                         \
            __VA_ARGS__;                                \
            *_result = (ret);                           \
            _exit(EXIT_SUCCESS);                        \
        } else {                                        \
            int _status;                                \
            waitpid(_pid, &_status, 0);                 \
            if (WIFSIGNALED(_status) &&                 \
                WTERMSIG(_status) == SIGABRT) {         \
                /* assert failure */                    \
                (code) = TEST_ASSERT_FAILURE;           \
            } else if (WIFEXITED(_status)) {            \
                (code) = WEXITSTATUS(_status);          \
                /* propagate child's own return code */ \
                (ret) = *_result;                       \
            } else {                                    \
                /* killed by other signal */            \
                (code) = EXIT_FAILURE;                  \
            }                                           \
            munmap(_result, sizeof(ret));               \
        }                                               \
    } while (0)
#else
    /* Skip assert-death tests or use a stub */
    #define RUN_ISOLATED(code, ret, ...) do { \
        (code) = SKIP_TEST;                   \
        memset(&(ret), 0, sizeof(ret));       \
    } while (0)
#endif

#define TEST_KEYMAP_COMPILE_FLAGS XKB_KEYMAP_COMPILE_STRICT_MODE

#define TEST_KEYMAP_SERIALIZE_FLAGS (    \
    XKB_KEYMAP_SERIALIZE_PRETTY |        \
    XKB_KEYMAP_SERIALIZE_KEEP_UNUSED     \
)

#define TEST_KEYMAP_SERIALIZE_EXPLICIT (           \
    XKB_KEYMAP_SERIALIZE_EXPLICIT_DEFAULT_VALUES | \
    XKB_KEYMAP_SERIALIZE_EXPLICIT_VMODS |          \
    XKB_KEYMAP_SERIALIZE_EXPLICIT_KEY_VALUES       \
)

void
test_init(void);

void
print_detailed_state(struct xkb_state *state);

/* The offset between KEY_* numbering, and keycodes in the XKB evdev
 * dataset. */
#define EVDEV_OFFSET 8

enum key_seq_state {
    DOWN,
    REPEAT,
    UP,
    BOTH,
    NEXT,
    FINISH,
};

int
test_key_seq(struct xkb_keymap *keymap, ...);

int
test_key_seq2(struct xkb_keymap *keymap, struct xkb_machine *sm,
              struct xkb_events *events, ...);

int
test_key_seq_va(struct xkb_keymap *keymap, struct xkb_machine * sm,
                struct xkb_events *events, va_list args);

char *
test_makedir(const char *parent, const char *path);

char *
test_maketempdir(const char *template);

char *
test_get_path(const char *path_rel);

char *
read_file(const char *path, FILE *file);

char *
test_read_file(const char *path_rel);

enum test_context_flags {
    CONTEXT_NO_FLAG = 0,
    CONTEXT_ALLOW_ENVIRONMENT_NAMES = (1 << 0),
};

struct xkb_context *
test_get_context(enum test_context_flags flags);

struct xkb_keymap *
test_compile_file(struct xkb_context *context, enum xkb_keymap_format format,
                  const char *path_rel);

struct xkb_keymap *
test_compile_string(struct xkb_context *context, enum xkb_keymap_format format,
                    const char *string);

struct xkb_keymap *
test_compile_buffer(struct xkb_context *context, enum xkb_keymap_format format,
                    const char *buf, size_t len);

struct xkb_keymap *
test_compile_buffer2(struct xkb_context *context, enum xkb_keymap_format format,
                     enum xkb_keymap_compile_flags flags,
                     const char *buf, size_t len);

typedef struct xkb_keymap * (*test_compile_buffer_t)(struct xkb_context *context,
                                                     enum xkb_keymap_format format,
                                                     const char *buf, size_t len,
                                                     void *private);

bool
test_compile_output(struct xkb_context *ctx, enum xkb_keymap_format input_format,
                    enum xkb_keymap_format output_format,
                    test_compile_buffer_t compile_buffer,
                    void *compile_buffer_private, const char *test_title,
                    const char *keymap_str, size_t keymap_len,
                    const char *rel_path, bool update_output_files);

bool
test_compile_output2(struct xkb_context *ctx,
                     enum xkb_keymap_format input_format,
                     enum xkb_keymap_format output_format,
                     enum xkb_keymap_serialize_flags serialize_flags,
                     test_compile_buffer_t compile_buffer,
                     void *compile_buffer_private, const char *test_title,
                     const char *keymap_str, size_t keymap_len,
                     const char *rel_path, bool update_output_files);

typedef int (*test_third_party_compile_buffer_t)(const char *buf, size_t len,
                                                 void *private,
                                                 char **out, size_t *size_out);

bool
test_third_pary_compile_output(test_third_party_compile_buffer_t compile_buffer,
                               void *compile_buffer_private,
                               const char *test_title,
                               const char *keymap_str, size_t keymap_len,
                               const char *rel_path, bool update_output_files);

struct xkb_keymap *
test_compile_rmlvo(struct xkb_context *context, enum xkb_keymap_format format,
                   const char *rules, const char *model, const char *layout,
                   const char *variant, const char *options);

struct xkb_keymap *
test_compile_rules(struct xkb_context *context, enum xkb_keymap_format format,
                   const char *rules, const char *model, const char *layout,
                   const char *variant, const char *options);

struct xkb_context_log_config_backup {
    xkb_context_log_fn_t fn;
    void *user_data;
    enum xkb_log_level level;
    int verbosity;
};

extern xkb_context_log_fn_t
xkb_context_get_log_fn(const struct xkb_context *ctx);

static inline void
xkb_context_log_backup_config(
    struct xkb_context * restrict ctx,
    struct xkb_context_log_config_backup * restrict backup
)
{
    backup->fn = xkb_context_get_log_fn(ctx);
    backup->user_data = xkb_context_get_user_data(ctx);
    backup->level = xkb_context_get_log_level(ctx);
    backup->verbosity = xkb_context_get_log_verbosity(ctx);
}

static inline void
xkb_context_log_restore_config(
    struct xkb_context * restrict ctx,
    const struct xkb_context_log_config_backup * restrict backup
)
{
    xkb_context_set_log_fn(ctx, backup->fn);
    xkb_context_set_user_data(ctx, backup->user_data);
    xkb_context_set_log_level(ctx, backup->level);
    xkb_context_set_log_verbosity(ctx, backup->verbosity);
}

typedef bool (*test_log_msg_func_t)(const char* log_entry, void *data);
struct test_log_msg_config {
    test_log_msg_func_t f;
    void *data;
};

static inline void
ATTR_PRINTF(3, 0)
test_log_msg(struct xkb_context *ctx, enum xkb_log_level lvl,
             const char *msg, va_list args)
{
    struct test_log_msg_config *test =
        (struct test_log_msg_config *)xkb_context_get_user_data(ctx);
    char * const log_entry = vasprintf_safe(msg, args);
    assert(log_entry);
    assert(test->f(log_entry, test->data));
    free(log_entry);
}

struct test_log_id_data {
    const char id[8];
    size_t count;
};

static inline bool
test_log_id_func(const char * restrict log_entry, void * restrict data)
{
    struct test_log_id_data *d = (struct test_log_id_data*)data;
    if (strstr(log_entry, d->id)) d->count++;
    return true;
}

static inline void
xkb_enable_quiet_logging(struct xkb_context *ctx)
{
    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(ctx, XKB_LOG_VERBOSITY_MINIMAL);
}

#ifdef _WIN32
#define setenv(varname, value, overwrite) _putenv_s((varname), (value))
#define unsetenv(varname) _putenv_s(varname, "")
#endif
