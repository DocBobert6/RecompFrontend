// [wcw] Unit tests for how radio options look and behave (src/elements/radio_rules). Built and run as part of every
// recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "radio_rules.h"

#include "wcw_test.h"

using namespace recompui::radio_rules;

TEST_CASE("a normal option is pickable whether or not it is current") {
    for (bool checked : { false, true }) {
        const OptionState s = option_state(true, false, checked);
        CHECK(!s.looks_disabled);
        CHECK(s.focusable);
        CHECK(s.pickable);
    }
}

TEST_CASE("a disabled option looks disabled and can't be focused, even when current") {
    for (bool locked : { false, true }) {
        for (bool checked : { false, true }) {
            const OptionState s = option_state(false, locked, checked);
            CHECK(s.looks_disabled);
            CHECK(!s.focusable);
            CHECK(!s.pickable);
        }
    }
}

TEST_CASE("a locked value that is current looks selected and takes focus, but can't be picked") {
    // The graphics preset's Custom: it was drawn greyed out while current, and controller focus landed on Authentic.
    const OptionState s = option_state(true, true, true);
    CHECK(!s.looks_disabled);
    CHECK(s.focusable);
    CHECK(!s.pickable);
}

TEST_CASE("a locked value that isn't current looks disabled and is skipped") {
    const OptionState s = option_state(true, true, false);
    CHECK(s.looks_disabled);
    CHECK(!s.focusable);
    CHECK(!s.pickable);
}
