#pragma once

#include <string>
#include <vector>

// [wcw] The Mods menu's kind tabs and its package Install (aki-roadmap#154 step 5). All lists every opened mod in mod
// order (and is the only tab that can reorder them); Add-ons lists those that carry code or a ROM patch; Texture
// packs lists the data packs. Model packs and Conversions open their own dialogs. Kept free of UI and librecomp types
// so it can be unit tested (recompui/tests).
namespace recompui::config::mod_tabs {
    enum class Tab { All, Addons, TexturePacks };

    // Whether a mod with these contents is listed on that tab.
    bool shows(Tab tab, bool code, bool rom_patch);
    // The first index whose flag is set, or -1.
    int first_visible(const std::vector<bool> &visible);

    // One installed or refused package, for the summary shown after Install.
    struct Outcome {
        std::string name;       // display name, or the file name when the package couldn't be read
        std::string outcome;    // librecomp's outcome_name: installed, upgraded, ..., refused, failed
        std::string message;    // librecomp's one line for the player
        std::vector<std::string> kept;   // edited files kept aside
    };
    // "Installed: A 1.0.0 ...\nNot installed: B (why)\n..." with the kept copies listed; true when anything failed
    // (the summary is then shown as an error).
    bool summarize(const std::vector<Outcome> &outcomes, std::string &text);
}
