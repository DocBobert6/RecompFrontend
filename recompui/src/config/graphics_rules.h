#pragma once

#include "ultramodern/config.hpp"

// [wcw] The Resolution / Downsampling / Supersampling and MSAA options' rules, kept free of UI state so they can be
// unit tested (recompui/tests). The graphics tab calls these; a change to the rules belongs here, with a test.
namespace recompui::graphics_rules {
    // How many times larger than the output the game renders before RT64 box-filters it down (1 = no downsampling).
    // Original and Original 2x take it from Downsampling Quality (ds_option: Off, 2x, 4x); Auto takes it from
    // Supersampling (ss_option: Off, 2x), which renders at twice the window's scale. Both options store Off as 0.
    int downsample_multiplier(ultramodern::renderer::Resolution resolution, int ds_option, int ss_option);

    // Whether an MSAA level can be picked on a device whose highest supported level is device_max.
    bool msaa_level_supported(ultramodern::renderer::Antialiasing level, ultramodern::renderer::Antialiasing device_max);
};
