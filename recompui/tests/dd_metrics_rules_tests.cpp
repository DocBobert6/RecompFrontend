// [wcw dd] Unit tests for the Datadog metrics switch (src/config/dd_metrics_rules, aki-roadmap#148). Built and run as
// part of every recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "dd_metrics_rules.h"

#include <cstring>

#include "wcw_test.h"

using namespace recompui::dd_metrics_rules;

TEST_CASE("dd metrics: off by default (no variable, setting off)") {
    CHECK(env_value_to_set(nullptr, false) == nullptr);
    CHECK(!metrics_on(nullptr, false));
}

TEST_CASE("dd metrics: the setting turns them on when the variable isn't set") {
    CHECK(std::strcmp(env_value_to_set(nullptr, true), "1") == 0);
    CHECK(metrics_on(nullptr, true));
    // An empty variable counts as unset (rt64 reads it as off), so the setting still applies.
    CHECK(std::strcmp(env_value_to_set("", true), "1") == 0);
    CHECK(metrics_on("", true));
    CHECK(!metrics_on("", false));
}

TEST_CASE("dd metrics: a set variable wins over the setting, both ways") {
    for (bool setting : { false, true }) {
        CHECK(env_value_to_set("1", setting) == nullptr);
        CHECK(env_value_to_set("0", setting) == nullptr);
        CHECK(metrics_on("1", setting));
        CHECK(metrics_on("yes", setting));
        CHECK(!metrics_on("0", setting));     // WCW_DD_METRICS=0 forces them off even with the setting on
    }
}
