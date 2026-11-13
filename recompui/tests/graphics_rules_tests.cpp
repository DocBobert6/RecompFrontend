// [wcw] Unit tests for the Resolution / Downsampling / Supersampling and MSAA rules (src/config/graphics_rules).
// Built and run as part of every recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "graphics_rules.h"

#include "wcw_test.h"

using ultramodern::renderer::Antialiasing;
using ultramodern::renderer::Resolution;
using namespace recompui::graphics_rules;

TEST_CASE("supersampling: Auto renders at twice the window's scale with Supersampling 2x") {
    CHECK_EQ(downsample_multiplier(Resolution::Auto, 0, 2), 2);
    CHECK_EQ(downsample_multiplier(Resolution::Auto, 0, 0), 1);
    // Downsampling Quality is for Original and Original 2x; at Auto it no longer forces 1x (it used to), but it
    // doesn't apply either: a saved 4x there doesn't turn on supersampling.
    CHECK_EQ(downsample_multiplier(Resolution::Auto, 4, 0), 1);
    CHECK_EQ(downsample_multiplier(Resolution::Auto, 4, 2), 2);
}

TEST_CASE("supersampling: Auto caps at 2x whatever the saved value") {
    CHECK_EQ(downsample_multiplier(Resolution::Auto, 0, 4), 2);
    CHECK_EQ(downsample_multiplier(Resolution::Auto, 0, 1), 1);
    CHECK_EQ(downsample_multiplier(Resolution::Auto, 0, -3), 1);
}

TEST_CASE("supersampling: Original and Original 2x keep Downsampling Quality and ignore Supersampling") {
    for (Resolution resolution : { Resolution::Original, Resolution::Original2x }) {
        CHECK_EQ(downsample_multiplier(resolution, 0, 2), 1);
        CHECK_EQ(downsample_multiplier(resolution, 2, 0), 2);
        CHECK_EQ(downsample_multiplier(resolution, 4, 2), 4);
    }
}

TEST_CASE("msaa: 8x is offered only on devices that support it") {
    CHECK(msaa_level_supported(Antialiasing::MSAA8X, Antialiasing::MSAA8X));
    CHECK(!msaa_level_supported(Antialiasing::MSAA8X, Antialiasing::MSAA4X));
    CHECK(!msaa_level_supported(Antialiasing::MSAA8X, Antialiasing::None));
    CHECK(msaa_level_supported(Antialiasing::MSAA4X, Antialiasing::MSAA8X));
    CHECK(!msaa_level_supported(Antialiasing::MSAA4X, Antialiasing::MSAA2X));
    CHECK(msaa_level_supported(Antialiasing::MSAA2X, Antialiasing::MSAA2X));
    CHECK(!msaa_level_supported(Antialiasing::MSAA2X, Antialiasing::None));
    CHECK(msaa_level_supported(Antialiasing::None, Antialiasing::None));
}
