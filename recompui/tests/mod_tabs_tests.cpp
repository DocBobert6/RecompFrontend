//
// recompui
//

// [wcw] Unit tests for the Mods menu's kind tabs and package Install summary (aki-roadmap#154 step 5).

#include "mod_tabs.h"

#include "wcw_test.h"

using namespace recompui::config::mod_tabs;

TEST_CASE("mod_tabs: which tab lists which mod") {
    CHECK(shows(Tab::All, true, false) && shows(Tab::All, false, false) && shows(Tab::All, false, true));
    CHECK(shows(Tab::Addons, true, false));
    CHECK(shows(Tab::Addons, false, true));      // a ROM-patch mod is an add-on, not a data pack
    CHECK(!shows(Tab::Addons, false, false));
    CHECK(shows(Tab::TexturePacks, false, false));
    CHECK(!shows(Tab::TexturePacks, true, false));
    CHECK(!shows(Tab::TexturePacks, false, true));
}

TEST_CASE("mod_tabs: first visible entry") {
    CHECK(first_visible({}) == -1);
    CHECK(first_visible({ false, false }) == -1);
    CHECK(first_visible({ false, true, true }) == 1);
}

TEST_CASE("mod_tabs: install summary") {
    std::string text;
    CHECK(!summarize({ { "Cam", "installed", "installed Cam 1.0.0 (addon)", {} } }, text));
    CHECK(text == "installed Cam 1.0.0 (addon)");
    CHECK(summarize({ { "Cam", "upgraded", "updated Cam 1.0.0 -> 1.1.0", { "kept/mods/cam.nrm.edited-20261029" } },
                      { "pack.zip", "refused", "it's for another game (vpw2)", {} } }, text));
    CHECK(text == "updated Cam 1.0.0 -> 1.1.0\n  your edited copy was kept: kept/mods/cam.nrm.edited-20261029\n"
                  "Not installed: it's for another game (vpw2) (pack.zip)");
}
