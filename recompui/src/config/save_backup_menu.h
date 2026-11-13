#pragma once

#include <string>
#include <vector>

#include "librecomp/save_backup_rules.hpp"

// [wcw saves] What General > Restore a Backup lists (aki-roadmap#50): one row per backup with its date and time in
// words, and which file it covers when that matters. Kept free of UI and file I/O so it can be unit tested
// (recompui/tests); the backup list itself comes from librecomp save_backups.hpp, newest first.
namespace recompui::config::save_backup_menu {
    using recomp::save_backups::Timestamp;

    // "Oct 15, 8:54 PM". The year is added when it isn't now's year ("Dec 31, 2025, 11:05 PM"), and the seconds when
    // with_seconds is set ("Oct 15, 8:54:07 PM").
    std::string display_time(const Timestamp &when, const Timestamp &now, bool with_seconds = false);

    struct Entry {
        Timestamp when;
        bool controller_pak = false;
        bool pending = false; // the restore already scheduled for the next start
    };

    struct Row {
        std::string time; // display_time, with seconds only where two rows would otherwise read the same
        std::string file; // "Game save" / "Controller Pak"; empty when every backup is of the same file
        std::string note; // "Restores at next start" for the pending one, else empty
    };

    // One row per entry, in the same order.
    std::vector<Row> rows(const std::vector<Entry> &entries, const Timestamp &now);

    // "game save" / "Controller Pak", for sentences.
    std::string file_name_in_text(bool controller_pak);

    // The confirmation question for restoring entry.
    std::string confirm_text(const Entry &entry, const Timestamp &now);
}
