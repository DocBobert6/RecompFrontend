// [wcw model import] Unit tests for the Model packs dialog's models.txt handling (src/config/model_pack_menu). Built
// and run as part of every recompui build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "model_pack_menu.h"

#include "wcw_test.h"

using namespace recompui::config::model_pack_menu;

TEST_CASE("pack ids are what the mod accepts") {
    CHECK(valid_pack_id("snake"));
    CHECK(valid_pack_id("banjo_190-hd"));
    CHECK(!valid_pack_id(""));
    CHECK(!valid_pack_id("Snake"));
    CHECK(!valid_pack_id("a b"));
    CHECK(!valid_pack_id("../x"));
    CHECK(!valid_pack_id(std::string(32, 'a')));
    CHECK(!valid_pack_id("off"));   // the mod's off keyword
    CHECK(valid_pack_id(std::string(31, 'a')));
}

TEST_CASE("slots read like the mod reads them") {
    const std::string text = "# test\nsnake   slot 1   # Snake\r\nbanjo slot 2\nbanjo slot 3\ncesium wrestler 0x0302\n";
    CHECK_EQ(pack_slot(text, "snake"), 1);
    CHECK_EQ(pack_slot(text, "banjo"), 2);   // the first line for it
    CHECK_EQ(pack_slot(text, "cesium"), 0);  // wrestler lines aren't slots
    CHECK(slot_pack(text, 1) == "snake");
    CHECK(slot_pack(text, 3) == "banjo");
    CHECK(slot_pack(text, 4) == "");
    CHECK_EQ(pack_slot(text + "off banjo\n", "banjo"), 0);   // turned off
    CHECK(slot_pack(text + "off banjo\n", 2) == "");
    CHECK_EQ(pack_slot("snake slot 5\nsnake slot 0\nsnake slot\n", "snake"), 0);   // not slots 1-4
    CHECK_EQ(pack_slot("snake slot 01\n", "snake"), 1);   // numbers as the mod reads them
    CHECK_EQ(pack_slot("snake slot 0x2\n", "snake"), 2);
}

TEST_CASE("a slot's first line decides, even when its pack is off (the mod doesn't look further)") {
    const std::string text = "a slot 1\nb slot 1\noff a\n";
    CHECK(slot_pack(text, 1) == "");
    CHECK_EQ(pack_slot(text, "b"), 0);
    CHECK_EQ(pack_slot("a slot 1\nb slot 1\n", "b"), 0);   // slot 1 is a's
    CHECK_EQ(pack_slot("a slot 1\na slot 2\n", "a"), 1);
}

TEST_CASE("assigning a slot moves the pack and takes the slot from the other pack") {
    const std::string text = "# keep me\nsnake slot 1\nbanjo slot 2\ncesium wrestler 0x0302 costume 0\n";
    const std::string moved = assign(text, "snake", 2);
    CHECK(moved == "# keep me\nsnake slot 2\ncesium wrestler 0x0302 costume 0\n");   // where the old line was
    CHECK_EQ(pack_slot(moved, "snake"), 2);
    CHECK_EQ(pack_slot(moved, "banjo"), 0);
    CHECK(slot_pack(moved, 1) == "");
}

TEST_CASE("off clears only the pack's slot lines") {
    const std::string text = "snake slot 1\nsnake wrestler 0x0302\nbanjo slot 2\n";
    CHECK(assign(text, "snake", 0) == "snake wrestler 0x0302\nbanjo slot 2\n");
    CHECK(assign("", "snake", 0) == "");
}

TEST_CASE("a pack turned off with an off line comes back on when given a slot") {
    const std::string text = "snake slot 1\noff snake\n";
    const std::string on = assign(text, "snake", 3);
    CHECK(on == "snake slot 3\n");
    CHECK_EQ(pack_slot(on, "snake"), 3);
    CHECK(assign(text, "snake", 0) == "off snake\n");   // Off leaves the off line
}

TEST_CASE("an empty or new file, a BOM, CR line ends and a missing last line end") {
    CHECK(assign("", "snake", 1) == "snake slot 1\n");
    CHECK(assign("\xEF\xBB\xBFsnake slot 1", "banjo", 2) == "snake slot 1\nbanjo slot 2\n");
    CHECK(assign("a slot 1\rb slot 2\r", "c", 1) == "c slot 1\nb slot 2\n");
    CHECK(assign("snake slot 01\nx wrestler 3\n", "banjo", 1) == "banjo slot 1\nx wrestler 3\n");   // 01 is slot 1
}

TEST_CASE("pack.json fields, with the id as the fallback name") {
    const Pack p = parse_pack_json("snake", R"({"schema": 1, "name": "Solid Snake", "author": "someone", "game": "nomercy"})");
    CHECK(p.id == "snake");
    CHECK(p.name == "Solid Snake");
    CHECK(p.author == "someone");
    CHECK(p.game == "nomercy");
    CHECK(parse_pack_json("banjo", "").name == "banjo");
    CHECK(parse_pack_json("banjo", "{not json").name == "banjo");
    CHECK(parse_pack_json("banjo", R"({"name": 5})").name == "banjo");
}

TEST_CASE("rows sort by name and say when models.txt does more than the dialog shows") {
    const std::vector<Pack> packs = { { "zed", "Zed", "", "" }, { "banjo", "Banjo", "", "" }, { "snake", "Solid Snake", "", "" } };
    const std::string text = "snake slot 1\nbanjo wrestler 0x0003\noff zed\n";
    const std::vector<Row> r = rows(packs, text);
    CHECK_EQ(r.size(), size_t(3));
    CHECK(r[0].pack.id == "banjo");
    CHECK(r[1].pack.id == "snake");
    CHECK(r[2].pack.id == "zed");
    CHECK_EQ(r[1].slot, 1);
    CHECK(r[0].note == "Also replaces wrestlers by id (models.txt)");
    CHECK(r[1].note == "");
    CHECK(r[2].note == "Off in models.txt");
}
