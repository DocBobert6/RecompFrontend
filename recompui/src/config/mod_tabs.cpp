#include "mod_tabs.h"

namespace recompui::config::mod_tabs {
    bool shows(Tab tab, bool code, bool rom_patch) {
        switch (tab) {
            case Tab::All: return true;
            case Tab::Addons: return code || rom_patch;
            case Tab::TexturePacks: return !code && !rom_patch;
        }
        return true;
    }

    int first_visible(const std::vector<bool> &visible) {
        for (size_t i = 0; i < visible.size(); i++) {
            if (visible[i]) {
                return int(i);
            }
        }
        return -1;
    }

    bool summarize(const std::vector<Outcome> &outcomes, std::string &text) {
        text.clear();
        bool failed = false;
        for (const Outcome &o : outcomes) {
            const bool bad = (o.outcome == "refused") || (o.outcome == "failed");
            failed |= bad;
            if (!text.empty()) {
                text += '\n';
            }
            text += (bad ? "Not installed: " : "") + (o.message.empty() ? o.name : o.message);
            if (bad && !o.name.empty() && o.message.find(o.name) == std::string::npos) {
                text += " (" + o.name + ")";
            }
            for (const std::string &k : o.kept) {
                text += "\n  your edited copy was kept: " + k;
            }
        }
        return failed;
    }
}
