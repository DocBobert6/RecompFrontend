#pragma once

#include <string>
#include <vector>

// [wcw fullconv] The Mods menu's Conversions dialog (aki-roadmap#57): the conversions installed in <config>/conversions
// and which one the next start runs (conversions/active.txt, read and written by librecomp's conversion picker API).
// A choice applies from the next start: a conversion is set up when the game starts (its mods, its save folder).
// AKI Launcher's own choice (AKI_CONVERSION) still wins over this one for the launches it starts. Kept free of UI and
// librecomp types so it can be unit tested (recompui/tests).
namespace recompui::config::conversion_menu {
    struct Entry {
        std::string id;
        std::string name;
        bool patch = false, codes = false, saves = false, textures = false, rt = false;
        std::string error;   // why its aki.json can't be read ("" = fine); such an entry can't be chosen
    };

    // "ROM patch, codes, saves, textures, ray tracing" for the components it has, "" for none.
    std::string components_summary(const Entry &entry);

    struct Row {
        Entry entry;           // empty id: the "Off" row (no conversion, the normal game)
        bool chosen = false;   // what conversions/active.txt names (Off when it names none)
        bool running = false;  // what this session runs (Off when it runs none)
        bool selectable = true;
        std::string note;      // shown under the name
    };

    // The rows: "Off" first, then the entries in the given order. chosen_id / running_id: "" = none. from_launcher: AKI
    // Launcher picked what runs now (AKI_CONVERSION); its restarts pick it again, so the choice here only applies to
    // starts without the launcher.
    std::vector<Row> rows(const std::vector<Entry> &entries, const std::string &chosen_id, const std::string &running_id,
        bool from_launcher);

    // The line above the list: what changes at the next start, if anything.
    std::string status_line(const std::vector<Entry> &entries, const std::string &chosen_id, const std::string &running_id,
        bool from_launcher);
}
