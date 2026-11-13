#pragma once

// [wcw fix] Rules for turning host input into N64 input, kept free of SDL so they can be unit tested
// (recompui/tests/input_rules_tests.cpp, aki-roadmap#305; the code is in recompinput/src/input_rules.cpp). input_state.cpp
// calls these; a change to the rules belongs here, with a test.
namespace recompinput::input_rules {
    // A stick or trigger bound to a button (the family's default puts the left stick on the N64 d-pad) presses at
    // analog_press_threshold and lets go only below analog_release_threshold. With one threshold a stick held near it,
    // or wobbling as it springs back, crossed it more than once per push: the game saw press, release, press, and menus
    // moved two rows (#305).
    constexpr float analog_press_threshold = 0.5f;
    constexpr float analog_release_threshold = 0.35f;

    // Whether the bound button is down now, given whether it was down at the last read and the axis value (0..1, the
    // bound direction).
    bool analog_as_digital(bool was_pressed, float value);
}
