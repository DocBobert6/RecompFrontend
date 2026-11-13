// [wcw fullconv] Unit tests for the Conversions dialog's rows and status line (src/config/conversion_menu). Built and
// run as part of every recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "conversion_menu.h"

#include "wcw_test.h"

using namespace recompui::config::conversion_menu;

namespace {
    std::vector<Entry> installed() {
        Entry aew;
        aew.id = "aew64";
        aew.name = "AEW 64";
        aew.codes = aew.saves = aew.textures = true;
        Entry revse;
        revse.id = "revse";
        revse.name = "Starrcade Edition 2.0";
        revse.patch = true;
        Entry broken;
        broken.id = "broken";
        broken.name = "broken";
        broken.error = "aki.json: missing \"components\"";
        return { aew, revse, broken };
    }
}

TEST_CASE("conversions: components read as a short list") {
    const std::vector<Entry> e = installed();
    CHECK(components_summary(e[0]) == "codes, saves, textures");
    CHECK(components_summary(e[1]) == "ROM patch");
    Entry all;
    all.patch = all.codes = all.saves = all.textures = all.rt = true;
    CHECK(components_summary(all) == "ROM patch, codes, saves, textures, ray tracing");
    CHECK(components_summary(Entry{}).empty());
}

TEST_CASE("conversions: Off comes first and is chosen and running when nothing is") {
    const std::vector<Row> r = rows(installed(), "", "", false);
    CHECK(r.size() == 4);
    CHECK(r[0].entry.id.empty());
    CHECK(r[0].entry.name == "Off");
    CHECK(r[0].chosen);
    CHECK(r[0].running);
    CHECK(r[0].note.empty());
    CHECK(!r[1].chosen && !r[1].running);
    CHECK(status_line(installed(), "", "", false) == "Playing the normal game.");
}

TEST_CASE("conversions: a choice for the next start says so until the game restarts (#57)") {
    const std::vector<Row> r = rows(installed(), "aew64", "", false);
    CHECK(!r[0].chosen);
    CHECK(r[0].running);
    CHECK(r[0].note == "Running now; the next start runs AEW 64");
    CHECK(r[1].chosen);
    CHECK(r[1].note == "From the next start");
    CHECK(status_line(installed(), "aew64", "", false) == "Restart the game to switch to AEW 64.");
    // After the restart the same choice is running.
    const std::vector<Row> after = rows(installed(), "aew64", "aew64", false);
    CHECK(after[1].running && after[1].chosen);
    CHECK(after[1].note == "Running now");
    CHECK(status_line(installed(), "aew64", "aew64", false) == "Playing AEW 64.");
    // And back to Off.
    CHECK(status_line(installed(), "", "aew64", false) == "Restart the game to switch to the normal game.");
}

TEST_CASE("conversions: an unreadable conversion is listed but can't be chosen") {
    const std::vector<Row> r = rows(installed(), "", "", false);
    CHECK(!r[3].selectable);
    CHECK(r[3].note == "Can't be played: aki.json: missing \"components\"");
    CHECK(r[1].selectable && r[2].selectable && r[0].selectable);
}

TEST_CASE("conversions: a launcher run doesn't promise a restart switches (the launcher picks again) (#57 review)") {
    // AKI Launcher started revse; the Mods menu's choice (for starts without the launcher) is aew64.
    const std::vector<Row> r = rows(installed(), "aew64", "revse", true);
    CHECK(r[2].running && !r[2].chosen);
    CHECK(r[2].note == "Running now (picked by AKI Launcher)");
    CHECK(r[1].chosen && r[1].note == "For starts without AKI Launcher");
    CHECK(status_line(installed(), "aew64", "revse", true) ==
        "AKI Launcher started Starrcade Edition 2.0. Starts without the launcher run AEW 64.");
    CHECK(status_line(installed(), "", "revse", true) ==
        "AKI Launcher started Starrcade Edition 2.0. Starts without the launcher run the normal game.");
    // The launcher's pick and the choice agree: nothing to explain.
    CHECK(status_line(installed(), "revse", "revse", true) == "Playing Starrcade Edition 2.0.");
    CHECK(rows(installed(), "revse", "revse", true)[2].note == "Running now");
}
