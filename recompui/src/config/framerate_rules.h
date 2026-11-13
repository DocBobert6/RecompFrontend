#pragma once

#include <cstdint>

#include "ultramodern/config.hpp"

// [wcw hfr] The Framerate option's rules, kept free of UI state so they can be unit tested (recompui/tests). The menu
// offers 30 (Original), the game's smooth rate (stored as "Manual") and twice the smooth rate (stored as "Smooth2x",
// held in the Display slot while the menu is open). The graphics tab calls these; a change to the rules belongs
// here, with a test.
namespace recompui::framerate {
    // The value a saved rr_option loads as. Only a saved Smooth (Manual) or Smooth2x choice survives, and only when the
    // game has a smooth rate; everything else, including a stale Display from older builds, loads as Original.
    ultramodern::renderer::RefreshRate parse_saved_option(const nlohmann::json &saved, uint32_t smooth_rate);

    // Turns the menu's choice into what the renderer gets: Smooth runs at smooth_rate and the doubled choice at
    // 2 x smooth_rate, both as a Manual rate; without a smooth rate the framerate stays Original.
    void apply_choice(ultramodern::renderer::RefreshRate &option, int &manual_value, uint32_t smooth_rate);
};
