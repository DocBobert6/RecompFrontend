// [wcw fix] Unit tests for the host-to-N64 input rules (recompinput/src/input_rules, aki-roadmap#305). Built and run as
// part of every recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "recompinput/input_rules.h"

#include <vector>

#include "wcw_test.h"

using namespace recompinput::input_rules;

// Feeds a run of axis values through the rule, as the game's reads would, and counts the presses it sees.
static int count_presses(const std::vector<float> &values) {
    bool down = false;
    int presses = 0;
    for (float v : values) {
        const bool now = analog_as_digital(down, v);
        if (now && !down) {
            presses++;
        }
        down = now;
    }
    return presses;
}

TEST_CASE("input: a stick press goes down at the press threshold and up below the release threshold") {
    CHECK(!analog_as_digital(false, 0.0f));
    CHECK(!analog_as_digital(false, 0.49f));
    CHECK(analog_as_digital(false, 0.5f));
    CHECK(analog_as_digital(true, 0.49f));
    CHECK(analog_as_digital(true, analog_release_threshold));
    CHECK(!analog_as_digital(true, 0.34f));
    CHECK(!analog_as_digital(true, 0.0f));
    CHECK(analog_release_threshold < analog_press_threshold);
}

TEST_CASE("input: a stick wobbling around the threshold is one press, not several") {
    // Regression for #305: a push that settles near 0.5 (or wobbles back through it on release) was press, release,
    // press with the old single threshold.
    CHECK_EQ(count_presses({ 0.0f, 0.3f, 0.52f, 0.47f, 0.53f, 0.48f, 0.51f, 0.2f, 0.0f }), 1);
    CHECK_EQ(count_presses({ 0.0f, 1.0f, 1.0f, 0.45f, 0.55f, 0.4f, 0.0f }), 1);
}

TEST_CASE("input: separate pushes still count") {
    CHECK_EQ(count_presses({ 0.0f, 1.0f, 0.0f, 1.0f, 0.1f, 0.9f, 0.0f }), 3);
    // Coming back only to 0.3 is a release, so a second push from there is a second press.
    CHECK_EQ(count_presses({ 0.0f, 0.8f, 0.3f, 0.8f, 0.0f }), 2);
}
