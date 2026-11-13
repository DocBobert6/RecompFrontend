// [wcw fix] Unit tests for the controls save/load rules (recompinput/src/controls_rules, aki-roadmap#303). Built and
// run as part of every recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "recompinput/controls_rules.h"

#include "wcw_test.h"

using namespace recompinput::controls_rules;

TEST_CASE("controls: a GUID hash is never 0 and is rebuilt the same from the saved fields") {
    // Regression for #303: a GUID read back from controls.json kept hash 0, so every saved pad shared one map entry.
    const uint64_t pad360 = controller_guid_hash("", 1118, 654, 0, 6554);
    const uint64_t padOne = controller_guid_hash("", 1118, 767, 0, 36243);
    CHECK(pad360 != 0);
    CHECK(padOne != 0);
    CHECK(pad360 != padOne);
    CHECK(controller_guid_hash("", 1118, 654, 0, 6554) == pad360);
    CHECK(controller_guid_hash("", 0, 0, 0, 0) != 0);
}

TEST_CASE("controls: each field changes the hash") {
    const uint64_t base = controller_guid_hash("", 1, 2, 3, 4);
    CHECK(controller_guid_hash("", 9, 2, 3, 4) != base);
    CHECK(controller_guid_hash("", 1, 9, 3, 4) != base);
    CHECK(controller_guid_hash("", 1, 2, 9, 4) != base);
    CHECK(controller_guid_hash("", 1, 2, 3, 9) != base);
    // Fields don't swap into each other.
    CHECK(controller_guid_hash("", 2, 1, 3, 4) != base);
}

TEST_CASE("controls: a saved pad keeps the link to its own profile") {
    const std::string a = "SERIAL__VID_1118_PID_654_VERSION_0_CRC16_6554";
    CHECK(saved_controller_profile_key(a, a, true, false, true) == a);
}

TEST_CASE("controls: a pre-#303 cross link goes back to the pad's own profile") {
    // The owner's files: pad 654 saved as linked to pad 767's profile.
    const std::string a = "SERIAL__VID_1118_PID_654_VERSION_0_CRC16_6554";
    const std::string b = "SERIAL__VID_1118_PID_767_VERSION_0_CRC16_36243";
    CHECK(saved_controller_profile_key(a, b, true, false, true) == a);
    // Without its own profile the entry is skipped; the pad gets its profile when it is plugged in.
    CHECK(saved_controller_profile_key(a, b, true, false, false).empty());
    // The single-player controller profile is not a pad's profile either.
    CHECK(saved_controller_profile_key(a, "controller_sp", true, false, true) == a);
}

TEST_CASE("controls: custom profiles keep their link, missing profiles don't") {
    const std::string a = "SERIAL__VID_1118_PID_654_VERSION_0_CRC16_6554";
    CHECK(saved_controller_profile_key(a, "my_layout", true, true, true) == "my_layout");
    CHECK(saved_controller_profile_key(a, "my_layout", false, true, true) == a);
    CHECK(saved_controller_profile_key(a, a, false, false, false).empty());
}

TEST_CASE("controls: changes are saved right away, but not mid-bind") {
    CHECK(!controls_need_save(0, 0, false));
    CHECK(controls_need_save(1, 0, false));
    CHECK(!controls_need_save(1, 0, true));   // waiting for the new input: save when the bind finishes
    CHECK(!controls_need_save(7, 7, true));
    CHECK(controls_need_save(3, 7, false));   // any difference counts, not only "newer"
}

TEST_CASE("controls: a GUID read from controls.json gets its hash back") {
    // Regression for #303: from_json left the hash at 0, so saved pads collapsed onto one controllers entry.
    const nlohmann::json saved = { {"serial", ""}, {"vendor", 1118}, {"product", 654}, {"version", 0}, {"crc16", 6554} };
    const recompinput::ControllerGUID guid = saved.get<recompinput::ControllerGUID>();
    CHECK(guid.hash != 0);
    CHECK(guid.hash == controller_guid_hash("", 1118, 654, 0, 6554));
    nlohmann::json other = saved;
    other["product"] = 767;
    other["crc16"] = 36243;
    CHECK(other.get<recompinput::ControllerGUID>().hash != guid.hash);
}

TEST_CASE("controls: two pads of one model with different serials get different hashes") {
    // Review of #303: the serial is in the profile key, so it must be in the hash too, or the second pad's link
    // overwrote the first's on load.
    const uint64_t a = controller_guid_hash("a4:53:85:00:00:01", 1356, 2508, 0x8111, 4660);
    const uint64_t b = controller_guid_hash("a4:53:85:00:00:02", 1356, 2508, 0x8111, 4660);
    CHECK(a != b);
    CHECK(controller_guid_hash("", 1356, 2508, 0x8111, 4660) != a);
    nlohmann::json saved = { {"serial", "a4:53:85:00:00:02"}, {"vendor", 1356}, {"product", 2508}, {"version", 0x8111}, {"crc16", 4660} };
    CHECK(saved.get<recompinput::ControllerGUID>().hash == b);
}
