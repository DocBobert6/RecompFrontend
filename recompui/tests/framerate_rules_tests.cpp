// [wcw] Unit tests for the Framerate option's rules (src/config/framerate_rules). Built and run as part of every
// recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "framerate_rules.h"

#include "wcw_test.h"

using ultramodern::renderer::RefreshRate;
using namespace recompui::framerate;

static int rateOf(RefreshRate option) {
    return static_cast<int>(option);
}

// What the renderer runs at for a saved choice: load it, then apply it as the menu does.
static void loadAndApply(const nlohmann::json &saved, uint32_t smooth_rate, RefreshRate &option, int &manual_value) {
    option = parse_saved_option(saved, smooth_rate);
    manual_value = 0;
    apply_choice(option, manual_value, smooth_rate);
}

TEST_CASE("a saved 120 (Smooth2x) loads and runs at 120") {
    RefreshRate option;
    int value;
    loadAndApply("Smooth2x", 60, option, value);
    CHECK_EQ(rateOf(option), rateOf(RefreshRate::Manual));
    CHECK_EQ(value, 120);
}

TEST_CASE("a saved Manual always runs at the smooth rate, whatever rr_manual_value says (120 was really 60)") {
    // "Manual" is the stored name of the 60 (Smooth) choice: writing rr_option = Manual with rr_manual_value = 120
    // into graphics.json gives 60, not 120. 120 is stored as "Smooth2x".
    RefreshRate option;
    int value;
    loadAndApply("Manual", 60, option, value);
    CHECK_EQ(rateOf(option), rateOf(RefreshRate::Manual));
    CHECK_EQ(value, 60);
    loadAndApply(static_cast<uint32_t>(RefreshRate::Manual), 60, option, value);
    CHECK_EQ(rateOf(option), rateOf(RefreshRate::Manual));
    CHECK_EQ(value, 60);
}

TEST_CASE("Original stays 30 (Original)") {
    RefreshRate option;
    int value;
    loadAndApply("Original", 60, option, value);
    CHECK_EQ(rateOf(option), rateOf(RefreshRate::Original));
}

TEST_CASE("a stale Display from older builds loads as Original") {
    CHECK_EQ(rateOf(parse_saved_option("Display", 60)), rateOf(RefreshRate::Original));
    CHECK_EQ(rateOf(parse_saved_option(static_cast<uint32_t>(RefreshRate::Display), 60)), rateOf(RefreshRate::Original));
}

TEST_CASE("unknown or malformed values load as Original") {
    CHECK_EQ(rateOf(parse_saved_option("Bogus", 60)), rateOf(RefreshRate::Original));
    CHECK_EQ(rateOf(parse_saved_option(nlohmann::json(), 60)), rateOf(RefreshRate::Original));
    CHECK_EQ(rateOf(parse_saved_option(true, 60)), rateOf(RefreshRate::Original));
}

TEST_CASE("games without frame interpolation always run at Original") {
    RefreshRate option;
    int value;
    for (const char *saved : { "Manual", "Smooth2x", "Display", "Original" }) {
        loadAndApply(saved, 0, option, value);
        CHECK_EQ(rateOf(option), rateOf(RefreshRate::Original));
    }

    option = RefreshRate::Manual;
    value = 0;
    apply_choice(option, value, 0);
    CHECK_EQ(rateOf(option), rateOf(RefreshRate::Original));
}

TEST_CASE("the menu's choices map to the renderer's rate") {
    RefreshRate option = RefreshRate::Manual;
    int value = 999;
    apply_choice(option, value, 60);
    CHECK_EQ(value, 60);
    option = RefreshRate::Display;
    apply_choice(option, value, 60);
    CHECK_EQ(rateOf(option), rateOf(RefreshRate::Manual));
    CHECK_EQ(value, 120);
    option = RefreshRate::Original;
    value = 7;
    apply_choice(option, value, 60);
    CHECK_EQ(rateOf(option), rateOf(RefreshRate::Original));
    CHECK_EQ(value, 7);
}

int main() {
    return WcwTest::runAll("recompui wcw tests");
}
