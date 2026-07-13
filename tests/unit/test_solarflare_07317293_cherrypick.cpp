/**
 * @file tests/unit/test_solarflare_07317293_cherrypick.cpp
 * @brief Regression guard for the round-4 cherry-pick of
 *        07317293 fix(input): don't send ALT when right-alt is remapped
 *        to meta.
 *
 * Upstream commit 07317293 fixed a bug where, when the user remapped
 * right-alt to the Windows key (key_rightalt_to_key_win = true),
 * Sunshine would still send the original ALT key in addition to the
 * remapped WIN key. The fix:
 *
 *   1. Added `bool key_rightalt_to_key_win` to input_t (src/config.h)
 *      and removed the temporary `bool map_rightalt_to_win` local
 *      variable (src/config.cpp). The local was a UI-only setting
 *      but the input-handling code needed access to the value.
 *   2. Added `bool left_alt_pressed` and `bool right_alt_pressed`
 *      member variables (src/input.cpp) to track the actual key
 *      state on the host.
 *   3. In the keypress handler, skip the ALT send for keyCode
 *      VKEY_RMENU when right_alt_pressed is true (i.e. the
 *      remap is active).
 *
 * The cherry-pick was applied in round 4 with auto-merge (the fork
 * hadn't touched any of the affected files).
 *
 * These tests are build-time guards: if a future commit drops the
 * `key_rightalt_to_key_win` field, reverts to the local variable,
 * or removes the left/right alt tracking, these tests fail with
 * a clear error message pointing at the round-4 cherry-pick.
 */
#include "../tests_common.h"

#include "src/file_handler.h"

#include <string>

namespace {

  std::string read_file(const std::string &path) {
    return file_handler::read_file(path.c_str());
  }

  bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
  }

}  // namespace

// =============================================================================
// 1. The input_t struct has the key_rightalt_to_key_win field.
//    The pre-07317293 input_t only had keyboard / mouse / controller;
//    the cherry-pick added this bool between keyboard and mouse.
// =============================================================================

TEST(SolarflareInputFixCherryPick, KeyRightaltToKeyWinFieldExists) {
  const auto content = read_file("src/config.h");
  ASSERT_FALSE(content.empty())
    << "Could not read src/config.h.";

  EXPECT_TRUE(contains(content, "bool key_rightalt_to_key_win"))
    << "src/config.h is missing 'bool key_rightalt_to_key_win' "
       "in the input_t struct. The 07317293 cherry-pick added "
       "this field so the input-handling code can read whether the "
       "user has remapped right-alt to the Windows key. Re-apply "
       "the cherry-pick.";

  // The field should be next to 'keyboard' / 'mouse' (the cherry-pick
  // inserted it between the two).
  const size_t keyboard_pos = content.find("bool keyboard;");
  const size_t key_rightalt_pos = content.find("bool key_rightalt_to_key_win;");
  const size_t mouse_pos = content.find("bool mouse;");
  ASSERT_NE(keyboard_pos, std::string::npos);
  ASSERT_NE(mouse_pos, std::string::npos);
  ASSERT_NE(key_rightalt_pos, std::string::npos);
  EXPECT_GT(key_rightalt_pos, keyboard_pos)
    << "key_rightalt_to_key_win is BEFORE the 'keyboard' field. "
       "The 07317293 cherry-pick inserted it between keyboard and "
       "mouse.";
  EXPECT_LT(key_rightalt_pos, mouse_pos)
    << "key_rightalt_to_key_win is AFTER the 'mouse' field. "
       "The 07317293 cherry-pick inserted it between keyboard and "
       "mouse.";
}

// =============================================================================
// 2. src/config.cpp uses the struct field directly (not the old
//    local variable). The pre-07317293 code had:
//      bool map_rightalt_to_win = false;
//      bool_f(vars, "key_rightalt_to_key_win", map_rightalt_to_win);
//      if (map_rightalt_to_win) { ... }
//    The cherry-pick replaced this with:
//      input.key_rightalt_to_key_win = false;
//      bool_f(vars, "key_rightalt_to_key_win", input.key_rightalt_to_key_win);
//      if (input.key_rightalt_to_key_win) { ... }
// =============================================================================

TEST(SolarflareInputFixCherryPick, ConfigCppUsesStructField) {
  const auto content = read_file("src/config.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/config.cpp.";

  // The old local variable MUST be gone.
  EXPECT_FALSE(contains(content, "bool map_rightalt_to_win = false"))
    << "src/config.cpp still has the old 'bool map_rightalt_to_win "
       "= false' local variable. The 07317293 cherry-pick removed "
       "this in favour of input.key_rightalt_to_key_win (a struct "
       "field) so the input handler can read the value. Re-apply "
       "the cherry-pick.";
  EXPECT_FALSE(contains(content, "bool_f(vars, \"key_rightalt_to_key_win\", map_rightalt_to_win)"))
    << "src/config.cpp still calls bool_f with the local "
       "map_rightalt_to_win variable. The 07317293 cherry-pick "
       "changed this to bool_f(vars, \"key_rightalt_to_key_win\", "
       "input.key_rightalt_to_key_win). Re-apply the cherry-pick.";

  // And the new struct-field usage MUST be present.
  EXPECT_TRUE(contains(content, "input.key_rightalt_to_key_win = false"))
    << "src/config.cpp is missing the 'input.key_rightalt_to_key_win "
       "= false' default-initialiser. The 07317293 cherry-pick "
       "moved the default from a local variable into the struct "
       "field. Re-apply the cherry-pick.";
  EXPECT_TRUE(contains(content, "bool_f(vars, \"key_rightalt_to_key_win\", input.key_rightalt_to_key_win)"))
    << "src/config.cpp's bool_f call still references the old "
       "local variable. The 07317293 cherry-pick changed the call "
       "to pass input.key_rightalt_to_key_win (the struct field) "
       "instead. Re-apply the cherry-pick.";
  EXPECT_TRUE(contains(content, "if (input.key_rightalt_to_key_win)"))
    << "src/config.cpp's 'if' condition for the remap is still "
       "checking the old local variable. The 07317293 cherry-pick "
       "changed it to 'if (input.key_rightalt_to_key_win)'. "
       "Re-apply the cherry-pick.";
}

// =============================================================================
// 3. src/input.cpp tracks left/right ALT state. The pre-07317293
//    code didn't have these booleans; the cherry-pick added them
//    so the keypress handler can skip the ALT send for VKEY_RMENU
//    when right_alt_pressed is true.
// =============================================================================

TEST(SolarflareInputFixCherryPick, InputTracksLeftRightAlt) {
  const auto content = read_file("src/input.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/input.cpp.";

  EXPECT_TRUE(contains(content, "bool left_alt_pressed = false"))
    << "src/input.cpp is missing the 'bool left_alt_pressed = false' "
       "member variable on the input struct. The 07317293 "
       "cherry-pick added this so the keypress handler can track "
       "the actual host-side state. Re-apply the cherry-pick.";
  EXPECT_TRUE(contains(content, "bool right_alt_pressed = false"))
    << "src/input.cpp is missing the 'bool right_alt_pressed = "
       "false' member variable. The 07317293 cherry-pick added "
       "this so the keypress handler can skip the ALT send for "
       "VKEY_RMENU when right-alt has been remapped. Re-apply "
       "the cherry-pick.";

  // The keypress handler must check both LMENU and RMENU codes.
  EXPECT_TRUE(contains(content, "keyCode == VKEY_LMENU"))
    << "src/input.cpp's keypress handler is missing the "
       "'keyCode == VKEY_LMENU' check. The 07317293 cherry-pick "
       "added this to update left_alt_pressed on LMENU events. "
       "Re-apply the cherry-pick.";
  EXPECT_TRUE(contains(content, "keyCode == VKEY_RMENU"))
    << "src/input.cpp's keypress handler is missing the "
       "'keyCode == VKEY_RMENU' check. The 07317293 cherry-pick "
       "added this to update right_alt_pressed on RMENU events "
       "and to skip the ALT send when the right-alt remap is active. "
       "Re-apply the cherry-pick.";
}
