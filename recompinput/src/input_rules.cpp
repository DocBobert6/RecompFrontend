#include "recompinput/input_rules.h"

namespace recompinput::input_rules {
    bool analog_as_digital(bool was_pressed, float value) {
        return value >= (was_pressed ? analog_release_threshold : analog_press_threshold);
    }
}
