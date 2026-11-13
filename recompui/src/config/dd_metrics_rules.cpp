#include "dd_metrics_rules.h"

#include <cstring>

namespace recompui::dd_metrics_rules {
    const char *env_value_to_set(const char *env_value, bool setting_on) {
        const bool env_set = (env_value != nullptr) && (env_value[0] != '\0');
        return (!env_set && setting_on) ? "1" : nullptr;
    }

    bool metrics_on(const char *env_value, bool setting_on) {
        const char *set = env_value_to_set(env_value, setting_on);
        const char *value = (set != nullptr) ? set : env_value;
        return (value != nullptr) && (value[0] != '\0') && (std::strcmp(value, "0") != 0);
    }
}
