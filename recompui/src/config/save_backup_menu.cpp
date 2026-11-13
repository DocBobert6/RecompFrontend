#include "save_backup_menu.h"

#include <cstdio>

namespace recompui::config::save_backup_menu {
    static const char *month_name(int month) {
        static const char *names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        return (month >= 1 && month <= 12) ? names[month - 1] : "???";
    }

    std::string display_time(const Timestamp &when, const Timestamp &now, bool with_seconds) {
        const int hour12 = (when.hour % 12 == 0) ? 12 : (when.hour % 12);
        const char *am_pm = (when.hour < 12) ? "AM" : "PM";

        char date[32];
        if (when.year == now.year) {
            snprintf(date, sizeof(date), "%s %d", month_name(when.month), when.day);
        }
        else {
            snprintf(date, sizeof(date), "%s %d, %d", month_name(when.month), when.day, when.year);
        }

        char time[32];
        if (with_seconds) {
            snprintf(time, sizeof(time), "%d:%02d:%02d %s", hour12, when.minute, when.second, am_pm);
        }
        else {
            snprintf(time, sizeof(time), "%d:%02d %s", hour12, when.minute, am_pm);
        }

        return std::string(date) + ", " + time;
    }

    static std::string file_label(bool controller_pak) {
        return controller_pak ? "Controller Pak" : "Game save";
    }

    std::vector<Row> rows(const std::vector<Entry> &entries, const Timestamp &now) {
        bool any_pak = false;
        bool any_main = false;
        for (const Entry &entry : entries) {
            (entry.controller_pak ? any_pak : any_main) = true;
        }
        const bool show_file = any_pak && any_main;

        std::vector<Row> out;
        out.reserve(entries.size());
        for (size_t i = 0; i < entries.size(); i++) {
            const Entry &entry = entries[i];
            // Two backups of the same file in the same minute (a launch backup right after a restore's) would read the
            // same; the seconds tell them apart.
            bool same_minute = false;
            for (size_t j = 0; j < entries.size(); j++) {
                const Entry &other = entries[j];
                if (j != i && (!show_file || other.controller_pak == entry.controller_pak) &&
                    display_time(other.when, now) == display_time(entry.when, now)) {
                    same_minute = true;
                    break;
                }
            }

            Row row;
            row.time = display_time(entry.when, now, same_minute);
            row.file = show_file ? file_label(entry.controller_pak) : "";
            row.note = entry.pending ? "Restores at next start" : "";
            out.push_back(row);
        }

        return out;
    }

    std::string file_name_in_text(bool controller_pak) {
        return controller_pak ? "Controller Pak" : "game save";
    }

    std::string confirm_text(const Entry &entry, const Timestamp &now) {
        return "Restore the " + file_name_in_text(entry.controller_pak) + " backup from " + display_time(entry.when, now) +
            "? It's restored the next time the game starts, and your current save is backed up first.";
    }
}
