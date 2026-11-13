#pragma once

// [wcw dd] The Datadog metrics switch (aki-roadmap#148). rt64's aki.game.* metrics and N64ModernRuntime's fatal
// breadcrumb read the environment variable WCW_DD_METRICS; the General tab's Datadog Metrics setting turns them on
// for a normal launch by setting it for the process at startup, before rt64 starts. Kept free of the config and the
// environment so it can be unit tested (recompui/tests).
namespace recompui::dd_metrics_rules {
    // The value to give WCW_DD_METRICS, or nullptr to leave it alone. env_value is the variable as the process got it
    // (nullptr when unset). A set, non-empty variable always wins, "0" included, so a launcher or script can force the
    // metrics on or off whatever the setting says; otherwise the setting turns them on with "1".
    const char *env_value_to_set(const char *env_value, bool setting_on);

    // Whether the metrics end up on, by the rule rt64 and the runtime read the variable with: set, not empty, not "0"
    // (rt64 WcwLogic::envSwitchOn, N64ModernRuntime recomp::dd::switch_on; keep the three the same).
    bool metrics_on(const char *env_value, bool setting_on);
}
