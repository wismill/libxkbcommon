// WARNING: This file was auto-generated by: scripts/update-symbols-tests.py
#include "evdev-scancodes.h"
#include <xkbcommon/xkbcommon.h>

#include "src/utils.h"
#include "test.h"


static void
test_merge_modes(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /* Mode: Augment */
    const char keymap_augment[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { include \"evdev\" };\n"
        "  xkb_types { include \"complete\" };\n"
        "  xkb_compat { include \"basic\" };\n"
        "  xkb_symbols { include \"pc+merge_modes(base)|merge_modes(new)\" };"
        "};";
    fprintf(stderr, "*** test_merge_modes: augment ***\n");
    keymap = test_compile_buffer(ctx, keymap_augment,
                                 ARRAY_SIZE(keymap_augment));
    assert(test_key_seq(keymap,
        KEY_Q, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_Q, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_W, BOTH, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_W, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_E, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_E, BOTH, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_R, BOTH, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_R, BOTH, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_T, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_T, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_Y, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_Y, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_U, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_U, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_I, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_I, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_A, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_A, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_S, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_S, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_D, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_D, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_F, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_F, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_G, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_G, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_H, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_H, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_J, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_J, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_K, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_K, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_L, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_L, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_SEMICOLON, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_SEMICOLON, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_APOSTROPHE, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_APOSTROPHE, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_BACKSLASH, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_BACKSLASH, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_Z, BOTH, XKB_KEY_NoSymbol, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_Z, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_X, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_X, BOTH, XKB_KEY_NoSymbol, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_C, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_C, BOTH, XKB_KEY_A, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_V, BOTH, XKB_KEY_a, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_V, BOTH, XKB_KEY_NoSymbol, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_B, BOTH, XKB_KEY_a, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_B, BOTH, XKB_KEY_C, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_N, BOTH, XKB_KEY_a, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_N, BOTH, XKB_KEY_A, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_M, BOTH, XKB_KEY_a, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_M, BOTH, XKB_KEY_A, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_GRAVE, BOTH, XKB_KEY_NoSymbol, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_GRAVE, BOTH, XKB_KEY_A, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_1, BOTH, XKB_KEY_c, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_1, BOTH, XKB_KEY_A, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_2, BOTH, XKB_KEY_a, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_2, BOTH, XKB_KEY_NoSymbol, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_3, BOTH, XKB_KEY_a, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_3, BOTH, XKB_KEY_NoSymbol, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_4, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_4, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_5, BOTH, XKB_KEY_a, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_5, BOTH, XKB_KEY_C, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_6, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_6, BOTH, XKB_KEY_A, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, FINISH
    ));
    xkb_keymap_unref(keymap);

    /* Mode: Override */
    const char keymap_override[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { include \"evdev\" };\n"
        "  xkb_types { include \"complete\" };\n"
        "  xkb_compat { include \"basic\" };\n"
        "  xkb_symbols { include \"pc+merge_modes(base)+merge_modes(new)\" };"
        "};";
    fprintf(stderr, "*** test_merge_modes: override ***\n");
    keymap = test_compile_buffer(ctx, keymap_override,
                                 ARRAY_SIZE(keymap_override));
    assert(test_key_seq(keymap,
        KEY_Q, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_Q, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_W, BOTH, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_W, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_E, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_E, BOTH, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_R, BOTH, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_R, BOTH, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_T, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_T, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_Y, BOTH, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_Y, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_U, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_U, BOTH, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_I, BOTH, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_I, BOTH, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_A, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_A, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_S, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_S, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_D, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_D, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_F, BOTH, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_F, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_G, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_G, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_H, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_H, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_J, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_J, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_K, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_K, BOTH, XKB_KEY_A, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_L, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_L, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_SEMICOLON, BOTH, XKB_KEY_a, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_SEMICOLON, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_APOSTROPHE, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_APOSTROPHE, BOTH, XKB_KEY_C, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_BACKSLASH, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_BACKSLASH, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_Z, BOTH, XKB_KEY_NoSymbol, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_Z, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_X, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_X, BOTH, XKB_KEY_NoSymbol, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_C, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_C, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_V, BOTH, XKB_KEY_c, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_V, BOTH, XKB_KEY_NoSymbol, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_B, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_B, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_N, BOTH, XKB_KEY_a, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_N, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_M, BOTH, XKB_KEY_c, XKB_KEY_b, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_M, BOTH, XKB_KEY_A, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_GRAVE, BOTH, XKB_KEY_NoSymbol, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_GRAVE, BOTH, XKB_KEY_A, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_1, BOTH, XKB_KEY_c, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_1, BOTH, XKB_KEY_C, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_2, BOTH, XKB_KEY_a, XKB_KEY_NoSymbol, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_2, BOTH, XKB_KEY_NoSymbol, XKB_KEY_B, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_3, BOTH, XKB_KEY_c, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_3, BOTH, XKB_KEY_C, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_4, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_4, BOTH, XKB_KEY_C, XKB_KEY_D, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_5, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_5, BOTH, XKB_KEY_C, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, NEXT,
        KEY_6, BOTH, XKB_KEY_c, XKB_KEY_d, NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L, NEXT,
        KEY_6, BOTH, XKB_KEY_C, NEXT,
        KEY_LEFTSHIFT, UP, XKB_KEY_Shift_L, FINISH
    ));
    xkb_keymap_unref(keymap);
}
