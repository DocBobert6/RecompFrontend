#include "radio_rules.h"

namespace recompui::radio_rules {
    OptionState option_state(bool enabled, bool locked, bool checked) {
        if (!enabled) {
            return { .looks_disabled = true, .focusable = false, .pickable = false };
        }

        if (locked) {
            return { .looks_disabled = !checked, .focusable = checked, .pickable = false };
        }

        return { .looks_disabled = false, .focusable = true, .pickable = true };
    }
}
