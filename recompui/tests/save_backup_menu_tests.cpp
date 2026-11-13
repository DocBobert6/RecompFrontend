// [wcw] Unit tests for the save backup list (src/config/save_backup_menu). Built and run as part of every recompui
// build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "save_backup_menu.h"

#include "wcw_test.h"

using namespace recompui::config::save_backup_menu;

static Timestamp at(int year, int month, int day, int hour, int minute, int second = 0) {
    Timestamp t;
    t.year = year; t.month = month; t.day = day; t.hour = hour; t.minute = minute; t.second = second;
    return t;
}

static const Timestamp now = at(2026, 10, 15, 21, 0);

TEST_CASE("dates read like Oct 15, 8:54 PM") {
    CHECK(display_time(at(2026, 10, 15, 20, 54, 7), now) == "Oct 15, 8:54 PM");
    CHECK(display_time(at(2026, 1, 3, 9, 5), now) == "Jan 3, 9:05 AM");
    CHECK(display_time(at(2026, 12, 31, 23, 59), now) == "Dec 31, 11:59 PM");
}

TEST_CASE("midnight and noon are 12, not 0") {
    CHECK(display_time(at(2026, 10, 15, 0, 0), now) == "Oct 15, 12:00 AM");
    CHECK(display_time(at(2026, 10, 15, 12, 30), now) == "Oct 15, 12:30 PM");
}

TEST_CASE("another year's date shows the year") {
    CHECK(display_time(at(2025, 12, 31, 23, 5), now) == "Dec 31, 2025, 11:05 PM");
}

TEST_CASE("seconds when asked") {
    CHECK(display_time(at(2026, 10, 15, 20, 54, 7), now, true) == "Oct 15, 8:54:07 PM");
}

TEST_CASE("rows keep the order and say nothing about the file when all backups are of the game save") {
    const std::vector<Row> r = rows({ { at(2026, 10, 15, 20, 54) }, { at(2026, 10, 14, 8, 1) } }, now);
    CHECK_EQ(r.size(), size_t(2));
    CHECK(r[0].time == "Oct 15, 8:54 PM");
    CHECK(r[1].time == "Oct 14, 8:01 AM");
    CHECK(r[0].file.empty());
    CHECK(r[1].file.empty());
    CHECK(r[0].note.empty());
}

TEST_CASE("with game save and Controller Pak backups each row says which file") {
    // A launch backs up both files in the same second; the file tells them apart, so no seconds are needed.
    const std::vector<Row> r = rows({
        { at(2026, 10, 15, 20, 54, 3), false },
        { at(2026, 10, 15, 20, 54, 3), true },
    }, now);
    CHECK(r[0].file == "Game save");
    CHECK(r[1].file == "Controller Pak");
    CHECK(r[0].time == "Oct 15, 8:54 PM");
    CHECK(r[1].time == "Oct 15, 8:54 PM");
}

TEST_CASE("two backups of the same file in the same minute get their seconds") {
    const std::vector<Row> r = rows({
        { at(2026, 10, 15, 20, 54, 40) },
        { at(2026, 10, 15, 20, 54, 2) },
        { at(2026, 10, 15, 20, 40, 0) },
    }, now);
    CHECK(r[0].time == "Oct 15, 8:54:40 PM");
    CHECK(r[1].time == "Oct 15, 8:54:02 PM");
    CHECK(r[2].time == "Oct 15, 8:40 PM");
}

TEST_CASE("the pending restore is marked") {
    const std::vector<Row> r = rows({ { at(2026, 10, 15, 20, 54), false, false }, { at(2026, 10, 14, 8, 1), false, true } }, now);
    CHECK(r[0].note.empty());
    CHECK(r[1].note == "Restores at next start");
}

TEST_CASE("no backups, no rows") {
    CHECK(rows({}, now).empty());
}

TEST_CASE("the confirmation names the file and the date") {
    CHECK(confirm_text({ at(2026, 10, 15, 20, 54) }, now) ==
        "Restore the game save backup from Oct 15, 8:54 PM? It's restored the next time the game starts, and your current save is backed up first.");
    CHECK(confirm_text({ at(2026, 10, 15, 20, 54), true }, now).find("Controller Pak backup from Oct 15, 8:54 PM?") != std::string::npos);
}
