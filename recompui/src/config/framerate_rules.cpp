#include "framerate_rules.h"

namespace recompui::framerate {
    using ultramodern::renderer::RefreshRate;

    RefreshRate parse_saved_option(const nlohmann::json &saved, uint32_t smooth_rate) {
        if (smooth_rate == 0) {
            return RefreshRate::Original;
        }

        const bool manual = saved.is_string() ? (saved.get<std::string>() == "Manual") :
            (saved.is_number() && (saved.get<uint32_t>() == static_cast<uint32_t>(RefreshRate::Manual)));
        if (manual) {
            return RefreshRate::Manual;
        }

        if (saved.is_string() && (saved.get<std::string>() == "Smooth2x")) {
            return RefreshRate::Display;
        }

        return RefreshRate::Original;
    }

    void apply_choice(RefreshRate &option, int &manual_value, uint32_t smooth_rate) {
        if (smooth_rate == 0) {
            option = RefreshRate::Original;
        }
        else if (option == RefreshRate::Display) {
            // The doubled smooth rate (Smooth2x).
            option = RefreshRate::Manual;
            manual_value = int(smooth_rate * 2);
        }
        else if (option == RefreshRate::Manual) {
            manual_value = int(smooth_rate);
        }
    }
};
