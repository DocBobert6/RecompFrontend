#include "conversion_menu.h"

namespace recompui::config::conversion_menu {
    std::string components_summary(const Entry &entry) {
        std::string out;
        auto add = [&out](bool has, const char *name) {
            if (!has) {
                return;
            }
            if (!out.empty()) {
                out += ", ";
            }
            out += name;
        };
        add(entry.patch, "ROM patch");
        add(entry.codes, "codes");
        add(entry.saves, "saves");
        add(entry.textures, "textures");
        add(entry.rt, "ray tracing");
        return out;
    }

    static std::string display_name(const std::vector<Entry> &entries, const std::string &id) {
        if (id.empty()) {
            return "the normal game";
        }
        for (const Entry &entry : entries) {
            if (entry.id == id) {
                return entry.name.empty() ? id : entry.name;
            }
        }
        return id;
    }

    std::vector<Row> rows(const std::vector<Entry> &entries, const std::string &chosen_id, const std::string &running_id,
        bool from_launcher) {
        std::vector<Row> out;
        Row off;
        off.entry.name = "Off";
        off.chosen = chosen_id.empty();
        off.running = running_id.empty();
        out.push_back(off);

        for (const Entry &entry : entries) {
            Row row;
            row.entry = entry;
            row.chosen = !chosen_id.empty() && (entry.id == chosen_id);
            row.running = !running_id.empty() && (entry.id == running_id);
            row.selectable = entry.error.empty();
            out.push_back(row);
        }

        for (Row &row : out) {
            if (!row.selectable) {
                row.note = "Can't be played: " + row.entry.error;
            }
            else if (row.running && row.chosen) {
                // The normal game running as chosen needs no note (the status line says it).
                row.note = row.entry.id.empty() ? std::string() : std::string("Running now");
            }
            else if (row.running) {
                row.note = from_launcher ? std::string("Running now (picked by AKI Launcher)") :
                    ("Running now; the next start runs " + display_name(entries, chosen_id));
            }
            else if (row.chosen) {
                row.note = from_launcher ? "For starts without AKI Launcher" : "From the next start";
            }
        }
        return out;
    }

    std::string status_line(const std::vector<Entry> &entries, const std::string &chosen_id, const std::string &running_id,
        bool from_launcher) {
        if (chosen_id == running_id) {
            return running_id.empty() ? "Playing the normal game." : ("Playing " + display_name(entries, running_id) + ".");
        }
        if (from_launcher) {
            return "AKI Launcher started " + display_name(entries, running_id) + ". Starts without the launcher run " +
                display_name(entries, chosen_id) + ".";
        }
        return "Restart the game to switch to " + display_name(entries, chosen_id) + ".";
    }
}
