#pragma once

#include <string>
#include <string_view>
#include <vector>

// [wcw model import] The Mods menu's Model packs dialog (aki-roadmap#130): the packs in <mods>/models and which wrestler
// slot each one replaces, read from and written to <mods>/models/models.txt (the aki_modelimport mod's assignments
// file; its rules: AkiMods mods/modelimport/README.md). Kept free of UI and file I/O so it can be unit tested
// (recompui/tests).
//
// The dialog only manages "<pack> slot N" lines: picking a slot for a pack drops every other line that gives that slot
// to a pack or this pack to a slot, and an "off <pack>" line for it; picking Off drops the pack's slot lines. Comments,
// wrestler / costume lines and everything else stay as they are.
namespace recompui::config::model_pack_menu {
    constexpr int slot_count = 4;

    // A pack id as the mod accepts it: a-z 0-9 _ -, 1 to 31 characters.
    bool valid_pack_id(std::string_view id);

    // The slot (1-4) a models.txt gives the pack with a "slot" line (the first such line), or 0. A pack turned off
    // ("off <pack>") has none.
    int pack_slot(const std::string &text, std::string_view pack_id);

    // The pack a models.txt's "slot" lines give a slot (1-4) - the first line for it whose pack isn't turned off - or "".
    std::string slot_pack(const std::string &text, int slot);

    // models.txt with the pack on slot (1-4), or on none (slot 0). Lines end in "\n"; a UTF-8 BOM is dropped.
    std::string assign(const std::string &text, std::string_view pack_id, int slot);

    struct Pack {
        std::string id;
        std::string name;    // pack.json "name", else the id
        std::string author;  // pack.json "author", else ""
        std::string game;    // pack.json "game", else ""
    };

    // A pack's display fields from its pack.json text (empty or invalid JSON: the id only).
    Pack parse_pack_json(std::string_view id, const std::string &pack_json);

    struct Row {
        Pack pack;
        int slot = 0;          // pack_slot
        std::string note;      // "Off in models.txt" / "Also replaces wrestlers by id" / ""
    };

    // One row per pack, sorted by name (then id).
    std::vector<Row> rows(const std::vector<Pack> &packs, const std::string &text);
}
