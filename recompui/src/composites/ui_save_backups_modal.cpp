#include "ui_save_backups_modal.h"

#include "librecomp/save_backups.hpp"
#include "recompui/recompui.h"
#include "elements/ui_modal.h"
#include "elements/ui_label.h"
#include "config/ui_config_option.h"
#include "config/save_backup_menu.h"

namespace recompui {
    namespace save_backups = recomp::save_backups;

    static Modal *backups_modal = nullptr;
    // What had focus before the dialog opened (the Restore a Backup button), to give it back on close.
    static Element *return_focus = nullptr;

    static void close_save_backups_modal() {
        if (backups_modal != nullptr && backups_modal->is_open_now()) {
            backups_modal->close();
        }
    }

    static void confirm_restore(const save_backups::BackupEntry &entry) {
        const config::save_backup_menu::Entry menu_entry{ entry.when, entry.controller_pak };
        recompui::open_choice_prompt(
            "Restore a Backup",
            config::save_backup_menu::confirm_text(menu_entry, save_backups::now()),
            "Restore",
            "Cancel",
            [entry]() {
                const bool ok = save_backups::request_restore(entry);
                close_save_backups_modal();
                recompui::open_info_prompt("Restore a Backup", ok ?
                    "Done: the backup is restored the next time the game starts." :
                    "Couldn't schedule the restore (check the save folder's permissions).",
                    "OK", {}, recompui::ButtonStyle::Primary);
            },
            {},
            recompui::ButtonStyle::Success, recompui::ButtonStyle::Danger, true);
    }

    // Fills the dialog for the backups on disk now. Must be called with the modal's context open; returns the element
    // to focus.
    static Element *fill_modal(ContextId context) {
        Element *header_left = backups_modal->get_header()->get_left();
        header_left->clear_children();
        header_left->set_padding(16.0f);
        header_left->set_padding_left(20.0f);
        context.create_element<Label>(header_left, "Restore a Backup", theme::Typography::Header3);

        Element *header_right = backups_modal->get_header()->get_right();
        header_right->clear_children();
        header_right->set_padding(16.0f);
        Button *close_button = context.create_element<Button>(header_right, "Close", ButtonStyle::Secondary, ButtonSize::Medium);
        close_button->add_pressed_callback(close_save_backups_modal);

        Element *body = backups_modal->get_body();
        body->clear_children();

        Element *column = context.create_element<Element>(body, 0, "div", false);
        column->set_display(Display::Flex);
        column->set_flex_direction(FlexDirection::Column);
        column->set_width(100.0f, Unit::Percent);
        column->set_padding(20.0f);
        column->set_padding_top(4.0f);
        column->set_gap(16.0f);
        column->set_as_navigation_container(NavigationType::Vertical);

        const std::filesystem::path main_save = save_backups::main_save_file();
        const std::vector<save_backups::BackupEntry> backups = save_backups::list_backups(main_save);
        if (backups.empty()) {
            context.create_element<Label>(column, "No backups yet", theme::Typography::LabelMD);
            Label *hint = context.create_element<Label>(column,
                "Your saves are backed up automatically when the game starts and while you play.", theme::Typography::Body);
            hint->set_color(theme::color::TextDim);
            return close_button;
        }

        Label *hint = context.create_element<Label>(column,
            "Pick a backup to restore. Newest first; the restore happens the next time the game starts.", theme::Typography::Body);
        hint->set_color(theme::color::TextDim);

        const std::optional<save_backups::BackupEntry> pending = save_backups::pending_restore(main_save);
        std::vector<config::save_backup_menu::Entry> entries;
        for (const save_backups::BackupEntry &backup : backups) {
            const bool is_pending = pending.has_value() && (pending->backup_file == backup.backup_file);
            entries.push_back({ backup.when, backup.controller_pak, is_pending });
        }
        const std::vector<config::save_backup_menu::Row> rows = config::save_backup_menu::rows(entries, save_backups::now());

        Element *list = context.create_element<Element>(column, 0, "div", false);
        list->set_display(Display::Flex);
        list->set_flex_direction(FlexDirection::Column);
        list->set_width(100.0f, Unit::Percent);
        list->set_max_height(560.0f);
        list->set_overflow_y(Overflow::Auto);
        list->set_gap(8.0f);
        list->set_padding(4.0f);
        list->set_as_navigation_container(NavigationType::Vertical);

        Element *first_row = nullptr;
        for (size_t i = 0; i < backups.size(); i++) {
            const config::save_backup_menu::Row &row = rows[i];
            ReportingButton *button = context.create_element<ReportingButton>(list, row.time, ButtonStyle::Tertiary, ButtonSize::Medium);
            button->set_width(100.0f, Unit::Percent);
            button->set_justify_content(JustifyContent::SpaceBetween);
            button->set_flex_shrink(0.0f);

            std::string detail = row.file;
            if (!row.note.empty()) {
                detail = detail.empty() ? row.note : (detail + " - " + row.note);
            }
            if (!detail.empty()) {
                Label *detail_label = context.create_element<Label>(button, detail, theme::Typography::LabelSM);
                detail_label->set_color(row.note.empty() ? theme::color::TextDim : theme::color::Primary);
            }

            const save_backups::BackupEntry entry = backups[i];
            button->add_pressed_callback([entry]() { confirm_restore(entry); });
            if (first_row == nullptr) {
                first_row = button;
            }
        }

        return first_row;
    }

    void open_save_backups_modal() {
        ContextId prev_context = try_close_current_context();
        return_focus = nullptr;
        if (prev_context != ContextId::null()) {
            // get_focused_element needs its context open.
            prev_context.open();
            return_focus = prev_context.get_focused_element();
            prev_context.close();
        }

        const bool created = (backups_modal == nullptr);
        if (created) {
            backups_modal = Modal::create_modal(ModalType::Prompt);
            backups_modal->set_on_close_callback([]() {
                if (return_focus != nullptr) {
                    return_focus->focus();
                    return_focus = nullptr;
                }
            });
        }

        ContextId modal_context = backups_modal->modal_root_context;
        modal_context.open();
        if (created) {
            // Renders the menu action hints, so it needs the modal's context open.
            backups_modal->set_menu_action_callback(MenuAction::Back, close_save_backups_modal, "Back");
        }
        Element *focus = fill_modal(modal_context);
        modal_context.close();

        if (prev_context != ContextId::null()) {
            prev_context.open();
        }
        backups_modal->open();
        if (focus != nullptr) {
            focus->focus();
        }
    }
}
