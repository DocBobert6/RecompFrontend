#pragma once

namespace recompui {
    // [wcw saves] General > Restore a Backup (aki-roadmap#50): a dialog listing the automatic save backups by date,
    // newest first. Picking one asks to confirm and schedules the restore for the next game start. Rows:
    // config/save_backup_menu.h; backups: librecomp save_backups.hpp.
    void open_save_backups_modal();
}
