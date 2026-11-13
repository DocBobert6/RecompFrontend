#include "graphics_rules.h"

#include <algorithm>

namespace recompui::graphics_rules {
    using ultramodern::renderer::Antialiasing;
    using ultramodern::renderer::Resolution;

    int downsample_multiplier(Resolution resolution, int ds_option, int ss_option) {
        switch (resolution) {
        case Resolution::Original:
        case Resolution::Original2x:
            return std::max(ds_option, 1);
        case Resolution::Auto:
        default:
            // Only 2x: Auto already renders at the window's size, so 4x would be 16 times its pixels.
            return (ss_option >= 2) ? 2 : 1;
        }
    }

    bool msaa_level_supported(Antialiasing level, Antialiasing device_max) {
        return static_cast<int>(level) <= static_cast<int>(device_max);
    }
};
