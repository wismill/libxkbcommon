/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef _XKBREGISTRY_H_
#define _XKBREGISTRY_H_

#include <stdarg.h>
#include <stdbool.h>

/**
 * @file
 * @brief Query for available RMLVO
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && !defined(__CYGWIN__)
# define RXKB_EXPORT      __attribute__((visibility("default")))
#elif defined(_WIN32)
# define RXKB_EXPORT      __declspec(dllexport)
#else
# define RXKB_EXPORT
#endif

/**
 * @defgroup registry Registry
 *
 * Querying for available rules, models, layouts, variants and options
 * ([RMLVO]).
 *
 * @note xkbregistry is part of the xkbcommon project but it is a separate
 * to its main library, (lib)xkbcommon.
 *
 * This library is the replacement for clients currently parsing `evdev.xml`
 * directly. The library is intended to provide easy access to the set of
 * **possible** [MLVO] configurations for a given ruleset. It is not a library
 * to apply these configurations, merely to enumerate them. The intended users
 * of this library are the configuration UIs that allow a user to select their
 * keyboard layout of choice.
 *
 * # Object lifetime
 *
 * All objects are *refcounted*. A context is created with an initial refcount
 * of 1: releasing it with `rxkb_context::rxkb_context_unref()` frees the
 * context together with every object it contains ([models], [layouts],
 * [option groups], [options] and [ISO 639]/[ISO 3166] codes). Objects are
 * therefore always freed with their context, never individually.
 *
 * The iteration functions (`rxkb_model::rxkb_model_first()`,
 * `rxkb_layout::rxkb_layout_next()`, etc.) return *borrowed* references:
 * the refcount of the returned object is *not* increased. To keep an object
 * beyond the immediate scope, take a reference with its `_ref()` function.
 * Taking the first reference on an object also keeps its ancestors — up to and
 * including the context — alive, and the object is only unpinned when you
 * release your last reference. Consequently, holding a reference on any object
 * guarantees that the whole context, and every other object in it, remains
 * valid until that reference is released.
 *
 * @warning Unref’ing an object that was never ref’ed is a programming error.
 *
 * [RMLVO]: @ref RMLVO-intro
 * [MLVO]: @ref RMLVO-intro
 * [models]: @ref rxkb_model
 * [layouts]: @ref rxkb_layout
 * [option groups]: @ref rxkb_option_group
 * [options]: @ref rxkb_option
 * [ISO 639]: @ref rxkb_iso639_code
 * [ISO 3166]: @ref rxkb_iso3166_code
 *
 * @{
 */

/**
 * @struct rxkb_context
 *
 * Opaque top level library context object.
 *
 * The context contains general library state, like include paths and parsed
 * data. Objects ([models], [layouts], [option groups], [options], etc.) are
 * created in a specific context, and multiple contexts may coexist
 * simultaneously.  Objects from different contexts are completely separated
 * and do not share any memory or state.
 *
 * @ref_counted_tree{rxkb_context,rxkb_context_parse}
 *
 * [models]: @ref rxkb_model
 * [layouts]: @ref rxkb_layout
 * [option groups]: @ref rxkb_option_group
 * [options]: @ref rxkb_option
 */
struct rxkb_context;

/**
 * @struct rxkb_model
 *
 * Opaque struct representing an XKB model.
 *
 * @ext_ref_counted{rxkb_model,rxkb_context,rxkb_context::rxkb_context_parse}
 */
struct rxkb_model;

/**
 * @struct rxkb_layout
 *
 * Opaque struct representing an XKB layout, including an optional variant.
 * Where the variant is `NULL`, the layout is the base layout.
 *
 * For example, `us` is the base layout, `us(intl)` is the `intl` variant of the
 * layout `us`.
 *
 * @ext_ref_counted{rxkb_layout,rxkb_context,rxkb_context::rxkb_context_parse}
 */
struct rxkb_layout;

/**
 * @struct rxkb_option_group
 *
 * Opaque struct representing an option group. Option groups divide the
 * individual options into logical groups. Their main purpose is to indicate
 * whether some options are mutually exclusive or not.
 *
 * @ext_ref_counted{rxkb_option_group,rxkb_context,rxkb_context::rxkb_context_parse}
 */
struct rxkb_option_group;

/**
 * @struct rxkb_option
 *
 * Opaque struct representing an XKB option. Options are grouped inside an
 * @ref rxkb_option_group.
 *
 * @ext_ref_counted{rxkb_option,rxkb_context,rxkb_context::rxkb_context_parse}
 */
struct rxkb_option;

/**
 * @struct rxkb_iso639_code
 *
 * @parblock
 * Opaque struct representing an ISO 639-3 code (e.g. `eng`, `fra`).
 * @endparblock
 *
 * There is no guarantee that two identical ISO codes share the same struct.
 * You must not rely on the pointer value of this struct.
 *
 * See https://iso639-3.sil.org/code_tables/639/data for a list of codes.
 *
 * @ext_ref_counted{rxkb_iso639_code,rxkb_context,rxkb_context::rxkb_context_parse}
 */
struct rxkb_iso639_code;

/**
 * @struct rxkb_iso3166_code
 *
 * @parblock
 * Opaque struct representing an ISO 3166 Alpha 2 code (e.g. `US`, `FR`).
 * @endparblock
 *
 * There is no guarantee that two identical ISO codes share the same struct.
 * You must not rely on the pointer value of this struct.
 *
 * See https://en.wikipedia.org/wiki/List_of_ISO_3166_country_codes for a list
 * of codes.
 *
 * @ext_ref_counted{rxkb_iso3166_code,rxkb_context,rxkb_context::rxkb_context_parse}
 */
struct rxkb_iso3166_code;

/**
 * @enum rxkb_popularity
 * Describes the popularity of an item. Historically, some highly specialized or
 * experimental definitions are excluded from the default list and shipped in
 * separate files. If these extra definitions are loaded
 * (see `::RXKB_CONTEXT_LOAD_EXOTIC_RULES`), the popularity of the item is set
 * accordingly.
 *
 * If the exotic items are not loaded, all items will have the standard
 * popularity.
 */
enum rxkb_popularity {
    RXKB_POPULARITY_STANDARD = 1,
    RXKB_POPULARITY_EXOTIC,
};

/**
 * @enum rxkb_context_flags
 * Flags for context creation.
 */
enum rxkb_context_flags {
    RXKB_CONTEXT_NO_FLAGS = 0,
    /**
     * Skip the default include paths. This requires the caller to call
     * `rxkb_context::rxkb_context_include_path_append()` or
     * `rxkb_context::rxkb_context_include_path_append_default()`.
     */
    RXKB_CONTEXT_NO_DEFAULT_INCLUDES = (1 << 0),
    /**
     * Load the extra items that are considered too exotic for the default list.
     *
     * For historical reasons, xkeyboard-config ships those exotic rules in a
     * separate file (e.g. `evdev.extras.xml`). Where the exotic rules are
     * requested, xkbregistry will look for and load `$ruleset.extras.xml`
     * in the include paths, see `rxkb_context::rxkb_context_include_path_append()`
     * for details on the lookup behavior.
     *
     * [xkeyboard-config]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config/
     */
    RXKB_CONTEXT_LOAD_EXOTIC_RULES = (1 << 1),
    /**
     * Disable the use of `secure_getenv()` for this context, so that privileged
     * processes can use environment variables. Client uses at their own risk.
     *
     * @since 1.5.0
     */
    RXKB_CONTEXT_NO_SECURE_GETENV = (1 << 2)
};

/**
 * Create a new xkb registry context.
 * @memberof rxkb_context
 *
 * The context has an initial refcount of 1. Use `rxkb_context_unref()` to
 * release memory associated with this context.
 *
 * Creating a context does not parse the files yet, use
 * `rxkb_context_parse()` or `rxkb_context_parse_default_ruleset()`.
 *
 * @param flags Flags affecting context behavior
 * @return A new xkb registry context or `NULL` on failure
 *
 * @sa `rxkb_context_unref()`
 * @sa `rxkb_context_parse()`
 * @sa `rxkb_context_parse_default_ruleset()`
 */
RXKB_EXPORT struct rxkb_context *
rxkb_context_new(enum rxkb_context_flags flags);

/**
 * @enum rxkb_log_level
 * Specifies a logging level.
 *
 * @sa `rxkb_context::rxkb_context_set_log_level()`
 * @sa `rxkb_context::rxkb_context_get_log_level()`
 */
enum rxkb_log_level {
    RXKB_LOG_LEVEL_CRITICAL = 10, /**< Log critical internal errors only. */
    RXKB_LOG_LEVEL_ERROR = 20,    /**< Log all errors. */
    RXKB_LOG_LEVEL_WARNING = 30,  /**< Log warnings and errors. */
    RXKB_LOG_LEVEL_INFO = 40,     /**< Log information, warnings, and errors. */
    RXKB_LOG_LEVEL_DEBUG = 50     /**< Log everything. */
};

/**
 * Set the current logging level.
 * @memberof rxkb_context
 *
 * @param ctx     The context in which to set the logging level.
 * @param level   The logging level to use.  Only messages from this level
 * and below will be logged.
 *
 * The default level is `::RXKB_LOG_LEVEL_ERROR`.  The environment variable
 * `RXKB_LOG_LEVEL`, if set at the time the context was created, overrides the
 * default value.  It may be specified as a level number or name.
 *
 * @sa `rxkb_context_get_log_level()`
 */
RXKB_EXPORT void
rxkb_context_set_log_level(struct rxkb_context *ctx,
                           enum rxkb_log_level level);

/**
 * Get the current logging level.
 * @memberof rxkb_context
 *
 * @sa `rxkb_context_set_log_level()`
 */
RXKB_EXPORT enum rxkb_log_level
rxkb_context_get_log_level(struct rxkb_context *ctx);

/**
 * Set a custom function to handle logging messages.
 * @memberof rxkb_context
 *
 * @param ctx     The context in which to use the set logging function.
 * @param log_fn  The function that will be called for logging messages.
 * Passing `NULL` restores the default function, which logs to `stderr`.
 *
 * By default, log messages from this library are printed to stderr.  This
 * function allows you to replace the default behavior with a custom
 * handler.  The handler is only called with messages which match the
 * current logging level and verbosity settings for the context.
 * level is the logging level of the message.  @a format and @a args are
 * the same as in the `vprintf(3)` function.
 *
 * You may use `rxkb_context_set_user_data()` on the context, and then call
 * `rxkb_context_get_user_data()` from within the logging function to provide
 * it with additional private context.
 */
RXKB_EXPORT void
rxkb_context_set_log_fn(struct rxkb_context *ctx,
                        void (*log_fn)(struct rxkb_context *ctx,
                                       enum rxkb_log_level level,
                                       const char *format, va_list args));


/**
 * Parse the given ruleset.
 * @memberof rxkb_context
 *
 * This can only be called once per context and once parsed the data in the
 * context is considered constant and will never change.
 *
 * This function parses all files with the given ruleset name. See
 * `rxkb_context_include_path_append()` for details.
 *
 * If this function returns false, xkbregistry failed to parse the XML files.
 * This is usually caused by invalid files on the host and should be debugged by
 * the host’s administrator using external tools. Callers should reduce the
 * include paths to known good paths and/or fall back to a default [RMLVO] set.
 *
 * If this function returns false, the context should be be considered dead and
 * must be released with `rxkb_context_unref()`.
 *
 * @param ctx The xkb registry context
 * @param ruleset The ruleset to parse, e.g. `evdev`
 * @return `true` on success or `false` on failure
 *
 * @sa `rxkb_context_parse_default_ruleset()`
 *
 * [RMLVO]: @ref RMLVO-intro
 */
RXKB_EXPORT bool
rxkb_context_parse(struct rxkb_context *ctx, const char *ruleset);

/**
 * Parse the default ruleset as configured at build time.
 * @memberof rxkb_context
 *
 * See `rxkb_context_parse()` for details.
 *
 * @sa `rxkb_context_parse()`
 */
RXKB_EXPORT bool
rxkb_context_parse_default_ruleset(struct rxkb_context *ctx);

/**
 * Take a new reference on a context.
 * @memberof rxkb_context
 *
 * Increases the refcount of this object by one and returns the object.
 *
 * @param ctx The xkb registry context
 * @return The passed-in object
 *
 * @sa `rxkb_context_unref()`
 */
RXKB_EXPORT struct rxkb_context*
rxkb_context_ref(struct rxkb_context *ctx);

/**
 * Release a reference on a context, and possibly free it.
 * @memberof rxkb_context
 *
 * Decreases the refcount of this object by one. Where the refcount hits zero,
 * associated resources will be freed.
 *
 * @param ctx The xkb registry context
 * @return always `NULL`
 *
 * @sa `rxkb_context_ref()`
 */
RXKB_EXPORT struct rxkb_context*
rxkb_context_unref(struct rxkb_context *ctx);

/**
 * Assign user-specific data.
 * @memberof rxkb_context
 *
 * xkbregistry will not look at or modify the data, it will merely return the
 * same pointer in `rxkb_context_get_user_data()`.
 *
 * @param ctx The xkb registry context
 * @param user_data User-specific data pointer
 *
 * @sa `rxkb_context_get_user_data()`
 */
RXKB_EXPORT void
rxkb_context_set_user_data(struct rxkb_context *ctx, void *user_data);

/**
 * Return the pointer passed into `rxkb_context_get_user_data()`.
 * @memberof rxkb_context
 *
 * @param ctx The xkb registry context
 * @return User-specific data pointer
 *
 * @sa `rxkb_context_set_user_data()`
 */
RXKB_EXPORT void *
rxkb_context_get_user_data(struct rxkb_context *ctx);

/**
 * Append a new entry to the context’s include path.
 * @memberof rxkb_context
 *
 * The include path handling is optimized for the most common use-case: a set of
 * system files that provide a complete set of [MLVO] and some
 * custom [MLVO] provided by a user **in addition** to the system set.
 *
 * The include paths should be given so that the least complete path is
 * specified first and the most complete path is appended last. For example:
 *
 * ```c
 * ctx = rxkb_context_new(RXKB_CONTEXT_NO_DEFAULT_INCLUDES);
 * rxkb_context_include_path_append(ctx, `/home/user/.config/xkb`);
 * rxkb_context_include_path_append(ctx, `/usr/share/X11/xkb`);
 * rxkb_context_parse(ctx, `evdev`);
 * ```
 *
 * The above example reflects the default behavior unless
 * `::RXKB_CONTEXT_NO_DEFAULT_INCLUDES` is provided.
 *
 * Loading of the files is in **reverse order**, i.e. the last path appended is
 * loaded first - in this case the ``/usr/share/X11/xkb`` path.
 * Any models, layouts, variants and options defined in the `evdev` ruleset
 * are loaded into the context. Then, any [RMLVO] found in the `evdev` ruleset of
 * the user’s path (``/home/user/.config/xkb`` in this example) are **appended**
 * to the existing set.
 *
 * Note that data from previously loaded include paths is never overwritten,
 * only appended to. It is not not possible to change the system-provided data,
 * only to append new models, layouts, variants and options to it.
 *
 * In other words, to define a new variant of the `us` layout called `banana`,
 * the following XML is sufficient.
 *
 * ```xml
 * <xkbConfigRegistry version="1.1">
 * <layoutList>
 *   <layout>
 *     <configItem>
 *       <name>us</name>
 *     </configItem>
 *     <variantList>
 *       <variant>
 *         <configItem>
 *          <name>banana</name>
 *          <description>English (Banana)</description>
 *        </configItem>
 *      </variant>
 *    </layout>
 * </layoutList>
 * </xkbConfigRegistry>
 * ```
 *
 * The list of models, options and all other layouts (including `us` and its
 * variants) is taken from the system files. The resulting list of layouts will
 * thus have a `us` keyboard layout with the variant `banana` and all other
 * system-provided variants (`dvorak`, `colemak`, `intl`, etc.)
 *
 * This function must be called before `rxkb_context_parse()` or
 * `rxkb_context_parse_default_ruleset()`.
 *
 * @returns `true` on success, or `false` if the include path could not be added
 * or is inaccessible.
 *
 * @sa `rxkb_context_include_path_append_default()`
 *
 * [MLVO]:  @ref RMLVO-intro
 * [RMLVO]: @ref RMLVO-intro
 */
RXKB_EXPORT bool
rxkb_context_include_path_append(struct rxkb_context *ctx, const char *path);

/**
 * Append the default include paths to the context’s include path.
 * @memberof rxkb_context
 *
 * See `rxkb_context_include_path_append()` for details about the merge order.
 *
 * This function must be called before `rxkb_context_parse()` or
 * `rxkb_context_parse_default_ruleset()`.
 *
 * @returns `true` on success, or `false` if the include path could not be added
 * or is inaccessible.
 *
 * @sa `rxkb_context_include_path_append()`
 */
RXKB_EXPORT bool
rxkb_context_include_path_append_default(struct rxkb_context *ctx);

/**
 * Return the first model for this context.
 * @memberof rxkb_model
 *
 * Use this to start iterating over the models, followed by calls to
 * `rxkb_model_next()`.
 *
 * @note Models are not sorted.
 *
 * @attention The refcount of the returned model is not increased.
 * Use `rxkb_model_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return The first model in the model list.
 *
 * @sa `struct rxkb_model`
 * @sa `rxkb_model_next()`
 */
RXKB_EXPORT struct rxkb_model *
rxkb_model_first(struct rxkb_context *ctx);

/**
 * Return the next model for this context.
 * @memberof rxkb_model
 *
 * @attention The refcount of the returned model is not increased.
 * Use `rxkb_model_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return the next model or `NULL` at the end of the list.
 *
 * @sa `struct rxkb_model`
 * @sa `rxkb_model_first()`
 */
RXKB_EXPORT struct rxkb_model *
rxkb_model_next(struct rxkb_model *m);

/**
 * Take a new reference on a model.
 * @memberof rxkb_model
 *
 * Increase the refcount of the argument by one. Taking the first reference on
 * the object also keeps its ancestors (up to and including the [context]) alive
 * until the last reference is released.
 *
 * @returns The argument passed in to this function.
 *
 * @sa `rxkb_model_unref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_model *
rxkb_model_ref(struct rxkb_model *m);

/**
 * Release a reference on a model, and possibly free it.
 * @memberof rxkb_model
 *
 * Decrease the refcount of the argument by one. When the last reference is
 * released, the object is no longer pinned to its parent; it is freed together
 * with its [context].
 *
 * @returns always `NULL`
 *
 * @sa `rxkb_model_ref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_model *
rxkb_model_unref(struct rxkb_model *m);

/**
 * Return the name of this model.
 * @memberof rxkb_model
 *
 * This is the value for M in [RMLVO], to be used with libxkbcommon.
 *
 * [RMLVO]: @ref RMLVO-intro
 */
RXKB_EXPORT const char *
rxkb_model_get_name(struct rxkb_model *m);

/**
 * Return a human-readable description of this model.
 * @memberof rxkb_model
 *
 * This function may return `NULL`.
 */
RXKB_EXPORT const char *
rxkb_model_get_description(struct rxkb_model *m);

/**
 * Return the vendor name for this model.
 * @memberof rxkb_model
 *
 * This function may return `NULL`.
 */
RXKB_EXPORT const char *
rxkb_model_get_vendor(struct rxkb_model *m);

/**
 * Return the popularity for this model.
 * @memberof rxkb_model
 *
 * @sa `enum rxkb_popularity`
 */
RXKB_EXPORT enum rxkb_popularity
rxkb_model_get_popularity(struct rxkb_model *m);

/**
 * Return the first layout for this context.
 * @memberof rxkb_layout
 *
 * Use this to start iterating over the layouts, followed by calls to
 * `rxkb_layout_next()`.
 *
 * @note Layouts are not sorted.
 *
 * @attention The refcount of the returned layout is not increased.
 * Use `rxkb_layout_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return The first layout in the layout list.
 *
 * @sa `rxkb_layout_next()`
 */
RXKB_EXPORT struct rxkb_layout *
rxkb_layout_first(struct rxkb_context *ctx);

/**
 * Return the next layout for this context.
 * @memberof rxkb_layout
 *
 * @attention The refcount of the returned layout is not increased.
 * Use `rxkb_layout_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return the next layout or `NULL` at the end of the list.
 *
 * @sa `rxkb_layout_first()`
 */
RXKB_EXPORT struct rxkb_layout *
rxkb_layout_next(struct rxkb_layout *l);

/**
 * Take a new reference on a layout.
 * @memberof rxkb_layout
 *
 * Increase the refcount of the argument by one. Taking the first reference on
 * the object also keeps its ancestors (up to and including the [context]) alive
 * until the last reference is released.
 *
 * @returns The argument passed in to this function.
 *
 * @sa `rxkb_layout_unref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_layout *
rxkb_layout_ref(struct rxkb_layout *l);

/**
 * Release a reference on a layout, and possibly free it.
 * @memberof rxkb_layout
 *
 * Decrease the refcount of the argument by one. When the last reference is
 * released, the object is no longer pinned to its parent; it is freed together
 * with its [context].
 *
 * @returns always `NULL`
 *
 * @sa `rxkb_layout_ref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_layout *
rxkb_layout_unref(struct rxkb_layout *l);

/**
 * Return the name of this layout.
 * @memberof rxkb_layout
 *
 * This is the value for L in [RMLVO], to be used with libxkbcommon.
 *
 * [RMLVO]: @ref RMLVO-intro
 */
RXKB_EXPORT const char *
rxkb_layout_get_name(struct rxkb_layout *l);

/**
 * Return the variant of this layout.
 * @memberof rxkb_layout
 *
 * This is the value for V in [RMLVO], to be used with libxkbcommon.
 *
 * A variant does not stand on its own, it always depends on the base layout.
 * E.g. there may be multiple variants called `intl` but there is only one
 * `us(intl)`.
 *
 * Where the variant is `NULL`, the layout is the base layout (e.g. `us`).
 *
 * [RMLVO]: @ref RMLVO-intro
 */
RXKB_EXPORT const char *
rxkb_layout_get_variant(struct rxkb_layout *l);

/**
 * Return a short (one-word) description of this layout.
 * @memberof rxkb_layout
 *
 * This function may return `NULL`.
 */
RXKB_EXPORT const char *
rxkb_layout_get_brief(struct rxkb_layout *l);

/**
 * Return a human-readable description of this layout.
 * @memberof rxkb_layout
 *
 * This function may return `NULL`.
 */
RXKB_EXPORT const char *
rxkb_layout_get_description(struct rxkb_layout *l);

/**
 * Return the popularity for this layout.
 * @memberof rxkb_layout
 */
RXKB_EXPORT enum rxkb_popularity
rxkb_layout_get_popularity(struct rxkb_layout *l);

/**
 * Return the first option group for this context.
 * @memberof rxkb_option_group
 *
 * Use this to start iterating over the option groups, followed by calls to
 * `rxkb_option_group_next()`.
 *
 * @note Option groups are not sorted.
 *
 * @attention The refcount of the returned option group is not increased. Use
 * `rxkb_option_group_ref()` if you need to keep this struct outside the immediate
 * scope.
 *
 * @return The first option group in the option group list.
 *
 * @sa `rxkb_option_group_next()`
 */
RXKB_EXPORT struct rxkb_option_group *
rxkb_option_group_first(struct rxkb_context *ctx);

/**
 * Return the next option group for this context.
 * @memberof rxkb_option_group
 *
 * @attention The refcount of the returned option group is not increased.
 * Use `rxkb_option_group_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return the next option group or `NULL` at the end of the list
 *
 * @sa `rxkb_option_group_first()`
 */
RXKB_EXPORT struct rxkb_option_group *
rxkb_option_group_next(struct rxkb_option_group *g);

/**
 * Take a new reference on an option group.
 * @memberof rxkb_option_group
 *
 * Increase the refcount of the argument by one. Taking the first reference on
 * the object also keeps its ancestors (up to and including the [context]) alive
 * until the last reference is released.
 *
 * @returns The argument passed in to this function.
 *
 * @sa `rxkb_option_group_unref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_option_group *
rxkb_option_group_ref(struct rxkb_option_group *g);

/**
 * Release a reference on an option group, and possibly free it.
 * @memberof rxkb_option_group
 *
 * Decrease the refcount of the argument by one. When the last reference is
 * released, the object is no longer pinned to its parent; it is freed together
 * with its [context].
 *
 * @returns always `NULL`
 *
 * @sa `rxkb_option_group_ref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_option_group *
rxkb_option_group_unref(struct rxkb_option_group *g);

/**
 * Return the name of this option group.
 * @memberof rxkb_option_group
 *
 * This is **not** the value for O in [RMLVO], the name can be used for internal
 * sorting in the caller. This function may return `NULL`.
 *
 * [RMLVO]: @ref RMLVO-intro
 */
RXKB_EXPORT const char *
rxkb_option_group_get_name(struct rxkb_option_group *g);

/**
 * Return a human-readable description of this option group.
 * @memberof rxkb_option_group
 *
 * This function may return `NULL`.
 */
RXKB_EXPORT const char *
rxkb_option_group_get_description(struct rxkb_option_group *g);

/**
 * @memberof rxkb_option_group
 *
 * @return `true` if multiple options within this option group can be selected
 *                simultaneously, `false` if all options within this option
 *                group are mutually exclusive.
 */
RXKB_EXPORT bool
rxkb_option_group_allows_multiple(struct rxkb_option_group *g);

/**
 * Return the popularity for this option group.
 * @memberof rxkb_option_group
 */
RXKB_EXPORT enum rxkb_popularity
rxkb_option_group_get_popularity(struct rxkb_option_group *g);

/**
 * Return the first option for this option group.
 * @memberof rxkb_option
 *
 * Use this to start iterating over the options, followed by calls to
 * `rxkb_option_next()`.
 *
 * @note Options are not sorted.
 *
 * @attention The refcount of the returned option is not increased.
 * Use `rxkb_option_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return The first option in the option list.
 *
 * @sa `rxkb_option_next()`
 */
RXKB_EXPORT struct rxkb_option *
rxkb_option_first(struct rxkb_option_group *group);

/**
 * Return the next option for this option group.
 * @memberof rxkb_option
 *
 * @attention The refcount of the returned options is not increased.
 * Use `rxkb_option_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @returns The next option or `NULL` at the end of the list
 *
 * @sa `rxkb_option_first()`
 */
RXKB_EXPORT struct rxkb_option *
rxkb_option_next(struct rxkb_option *o);

/**
 * Take a new reference on an option.
 * @memberof rxkb_option
 *
 * Increase the refcount of the argument by one. Taking the first reference on
 * the object also keeps its ancestors (up to and including the [context]) alive
 * until the last reference is released.
 *
 * @returns The argument passed in to this function.
 *
 * @sa `rxkb_option_unref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_option *
rxkb_option_ref(struct rxkb_option *o);

/**
 * Release a reference on an option, and possibly free it.
 * @memberof rxkb_option
 *
 * Decrease the refcount of the argument by one. When the last reference is
 * released, the object is no longer pinned to its parent; it is freed together
 * with its [context].
 *
 * @returns always `NULL`
 *
 * @sa `rxkb_option_ref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_option *
rxkb_option_unref(struct rxkb_option *o);

/**
 * Return the name of this option.
 * @memberof rxkb_option
 *
 * This is the value for O in [RMLVO], to be used with libxkbcommon.
 *
 * [RMLVO]: @ref RMLVO-intro
 */
RXKB_EXPORT const char *
rxkb_option_get_name(struct rxkb_option *o);

/**
 * Return a short (one-word) description of this option.
 * @memberof rxkb_option
 *
 * This function may return `NULL`.
 */
RXKB_EXPORT const char *
rxkb_option_get_brief(struct rxkb_option *o);

/**
 * Return a human-readable description of this option.
 * @memberof rxkb_option
 *
 * This function may return `NULL`.
 */
RXKB_EXPORT const char *
rxkb_option_get_description(struct rxkb_option *o);

/**
 * Return the popularity for this option.
 * @memberof rxkb_option
 */
RXKB_EXPORT enum rxkb_popularity
rxkb_option_get_popularity(struct rxkb_option *o);

/**
 * Return `true` if the given option accepts layout index specifiers to restrict
 * its application to the corresponding layouts, `false` otherwise.
 * @memberof rxkb_option
 *
 * @sa `xkb_rmlvo_builder::xkb_rmlvo_builder_append_layout()`
 * @sa `xkb_rule_names::options`
 *
 * @since 1.11.0
 */
RXKB_EXPORT bool
rxkb_option_is_layout_specific(struct rxkb_option *o);

/**
 * Take a new reference on an ISO 639 code.
 * @memberof rxkb_iso639_code
 *
 * Increase the refcount of the argument by one. Taking the first reference on
 * the object also keeps its ancestors (up to and including the [context]) alive
 * until the last reference is released.
 *
 * @returns The argument passed in to this function.
 *
 * @sa `rxkb_iso639_code_unref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_iso639_code *
rxkb_iso639_code_ref(struct rxkb_iso639_code *iso639);

/**
 * Release a reference on an ISO 639 code, and possibly free it.
 * @memberof rxkb_iso639_code
 *
 * Decrease the refcount of the argument by one. When the last reference is
 * released, the object is no longer pinned to its parent; it is freed together
 * with its [context].
 *
 * @returns always `NULL`
 *
 * @sa `rxkb_iso639_code_ref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_iso639_code *
rxkb_iso639_code_unref(struct rxkb_iso639_code *iso639);

/**
 * @parblock
 * Return the [ISO 639-3 code] for this code (e.g. `eng`, `fra`).
 * @endparblock
 * @memberof rxkb_iso639_code
 *
 * @sa `struct rxkb_iso639_code`
 *
 * [ISO 639-3 code]: @ref rxkb_iso639_code
 */
RXKB_EXPORT const char *
rxkb_iso639_code_get_code(struct rxkb_iso639_code *iso639);

/**
 * Return the first [ISO 639-3 code] for this layout.
 * @memberof rxkb_iso639_code
 *
 * Use this to start iterating over the codes, followed by calls to
 * `rxkb_iso639_code_next()`.
 *
 * @note Codes are not sorted.
 *
 * @attention The refcount of the returned code is not increased.
 * Use `rxkb_iso639_code_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return The first code in the code list.
 *
 * @sa `struct rxkb_iso639_code`
 * @sa `rxkb_iso639_code_next()`
 *
 * [ISO 639-3 code]: @ref rxkb_iso639_code
 */
RXKB_EXPORT struct rxkb_iso639_code *
rxkb_layout_get_iso639_first(struct rxkb_layout *layout);

/**
 * Return the next [ISO 639-3 code] in the list.
 * @memberof rxkb_iso639_code
 *
 * @attention The refcount of the returned codes is not increased.
 * Use `rxkb_iso639_code_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @returns The next code or `NULL` at the end of the list
 *
 * @sa `struct rxkb_iso639_code`
 * @sa `rxkb_layout_get_iso639_first()`
 *
 * [ISO 639-3 code]: @ref rxkb_iso639_code
 */
RXKB_EXPORT struct rxkb_iso639_code *
rxkb_iso639_code_next(struct rxkb_iso639_code *iso639);

/**
 * Take a new reference on an ISO 3166 code.
 * @memberof rxkb_iso3166_code
 *
 * Increase the refcount of the argument by one. Taking the first reference on
 * the object also keeps its ancestors (up to and including the [context]) alive
 * until the last reference is released.
 *
 * @returns The argument passed in to this function.
 *
 * @sa `rxkb_iso3166_code_unref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_iso3166_code *
rxkb_iso3166_code_ref(struct rxkb_iso3166_code *iso3166);

/**
 * Release a reference on an ISO 3166 code, and possibly free it.
 * @memberof rxkb_iso3166_code
 *
 * Decrease the refcount of the argument by one. When the last reference is
 * released, the object is no longer pinned to its parent; it is freed together
 * with its [context].
 *
 * @returns always `NULL`
 *
 * @sa `rxkb_iso3166_code_ref()`
 *
 * [context]: @ref rxkb_context
 */
RXKB_EXPORT struct rxkb_iso3166_code *
rxkb_iso3166_code_unref(struct rxkb_iso3166_code *iso3166);

/**
 * @parblock
 * Return the [ISO 3166 Alpha 2 code] for this code (e.g. `US`, `FR`).
 * @endparblock
 * @memberof rxkb_iso3166_code
 *
 * [ISO 3166 Alpha 2 code]: @ref rxkb_iso3166_code
 */
RXKB_EXPORT const char *
rxkb_iso3166_code_get_code(struct rxkb_iso3166_code *iso3166);

/**
 * Return the first [ISO 3166 code] for this layout.
 * @memberof rxkb_iso3166_code
 *
 * Use this to start iterating over the codes, followed by calls to
 * `rxkb_iso3166_code_next()`.
 *
 * @note Codes are not sorted.
 *
 * @attention The refcount of the returned code is not increased.
 * Use `rxkb_iso3166_code_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @return The first code in the code list.
 *
 * @sa `struct rxkb_iso3166_code`
 * @sa `rxkb_iso3166_code_next()`
 *
 * [ISO 3166 code]: @ref rxkb_iso3166_code
 */
RXKB_EXPORT struct rxkb_iso3166_code *
rxkb_layout_get_iso3166_first(struct rxkb_layout *layout);

/**
 * Return the next [ISO 3166 code] in the list.
 * @memberof rxkb_iso3166_code
 *
 * @attention The refcount of the returned codes is not increased.
 * Use `rxkb_iso3166_code_ref()` if you need to keep this struct outside
 * the immediate scope.
 *
 * @returns The next code or `NULL` at the end of the list
 *
 * @sa `struct rxkb_iso3166_code`
 * @sa `rxkb_layout_get_iso3166_first()`
 *
 * [ISO 3166 code]: @ref rxkb_iso3166_code
 */
RXKB_EXPORT struct rxkb_iso3166_code *
rxkb_iso3166_code_next(struct rxkb_iso3166_code *iso3166);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XKBREGISTRY_H_ */
