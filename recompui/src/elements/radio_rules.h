#pragma once

// [wcw] How one radio option looks and behaves. Kept free of UI state so it can be unit tested (recompui/tests);
// RadioOption calls it.
namespace recompui::radio_rules {
    struct OptionState {
        bool looks_disabled; // greyed out
        bool focusable;
        bool pickable;       // clicking or pressing it selects it
    };

    // enabled: the option (and its radio) is enabled. locked: the value can be shown but not picked, like the graphics
    // preset's Custom. checked: it is the current value.
    // A locked value that is current looks and focuses like any selected value (so controller focus lands on it, not
    // on its neighbour), but pressing it does nothing. A locked value that isn't current looks disabled.
    OptionState option_state(bool enabled, bool locked, bool checked);
}
