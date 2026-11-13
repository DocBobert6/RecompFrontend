#include "ui_conversions_modal.h"

#include <fstream>
#include <iterator>

#include "librecomp/conversion.hpp"
#include "recompui/recompui.h"
#include "elements/ui_modal.h"
#include "elements/ui_label.h"
#include "elements/ui_image.h"
#include "elements/ui_container.h"
#include "config/conversion_menu.h"

namespace recompui {
    namespace menu = config::conversion_menu;

    static Modal *conversions_modal = nullptr;
    // What had focus before the dialog opened (the Conversions button), to give it back on close.
    static Element *return_focus = nullptr;
    // Icon textures queued for the rows shown now, released when the dialog refills or closes.
    static std::vector<std::string> icons;
    static unsigned icon_generation = 0;   // in the texture names: the image cache never replaces a name
    // The rows shown now. A choice updates them in place: rebuilding the list from a button's own callback would
    // destroy that button while it runs.
    struct RowWidgets {
        std::string id;
        Button *choose = nullptr;
        Label *note = nullptr;
    };
    static std::vector<RowWidgets> row_widgets;
    static std::vector<menu::Entry> shown_entries;
    static std::vector<std::filesystem::path> shown_folders;
    static Label *status_label = nullptr;

    static std::string read_file(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    static void list_entries() {
        shown_entries.clear();
        shown_folders.clear();
        for (const recomp::mods::InstalledConversion &installed : recomp::mods::list_installed_conversions()) {
            menu::Entry entry;
            entry.id = installed.conversion.id;
            entry.name = installed.conversion.name;
            entry.patch = installed.conversion.has_patch;
            entry.codes = installed.conversion.has_codes;
            entry.saves = installed.conversion.has_saves;
            entry.textures = installed.conversion.has_textures;
            entry.rt = installed.conversion.has_rt;
            entry.error = installed.error;
            shown_entries.push_back(entry);
            shown_folders.push_back(installed.folder);
        }
    }

    static void release_icons() {
        for (const std::string &src : icons) {
            recompui::release_image(src);
        }
        icons.clear();
    }

    static void close_conversions_modal() {
        if (conversions_modal != nullptr && conversions_modal->is_open_now()) {
            conversions_modal->close();
        }
    }

    // Shows the current choice on the rows (highlighted button, notes) and the status line.
    static void show_rows() {
        const std::string chosen = recomp::mods::chosen_conversion();
        const std::string running = recomp::mods::session_conversion_id();
        const bool from_launcher = recomp::mods::session_conversion_from_launcher();
        const std::vector<menu::Row> rows = menu::rows(shown_entries, chosen, running, from_launcher);
        for (RowWidgets &w : row_widgets) {
            for (const menu::Row &row : rows) {
                if (row.entry.id != w.id) {
                    continue;
                }
                w.choose->apply_button_style(row.chosen ? ButtonStyle::Primary : ButtonStyle::Tertiary);
                w.choose->set_text(row.chosen ? "Chosen" : "Choose");
                w.note->set_text(row.note);
                w.note->set_display(row.note.empty() ? Display::None : Display::Block);
            }
        }
        if (status_label != nullptr) {
            status_label->set_text(menu::status_line(shown_entries, chosen, running, from_launcher));
        }
    }

    static void choose(const std::string &id) {
        if (!recomp::mods::set_chosen_conversion(id)) {
            recompui::open_info_prompt("Conversions", "Couldn't save the choice (check the conversions folder's permissions).",
                "OK", {}, recompui::ButtonStyle::Primary);
            return;
        }
        show_rows();
    }

    // Fills the dialog for the conversions on disk now. Must be called with the modal's context open; returns the
    // element to focus (the current choice's button).
    static Element *fill_modal(ContextId context) {
        release_icons();
        icon_generation++;
        row_widgets.clear();
        status_label = nullptr;

        Element *header_left = conversions_modal->get_header()->get_left();
        header_left->clear_children();
        header_left->set_padding(16.0f);
        header_left->set_padding_left(20.0f);
        context.create_element<Label>(header_left, "Conversions", theme::Typography::Header3);

        Element *header_right = conversions_modal->get_header()->get_right();
        header_right->clear_children();
        header_right->set_padding(16.0f);
        Button *close_button = context.create_element<Button>(header_right, "Close", ButtonStyle::Secondary, ButtonSize::Medium);
        close_button->add_pressed_callback(close_conversions_modal);

        Element *body = conversions_modal->get_body();
        body->clear_children();

        Element *column = context.create_element<Element>(body, 0, "div", false);
        column->set_display(Display::Flex);
        column->set_flex_direction(FlexDirection::Column);
        column->set_width(100.0f, Unit::Percent);
        column->set_padding(20.0f);
        column->set_padding_top(4.0f);
        column->set_gap(16.0f);
        column->set_as_navigation_container(NavigationType::Vertical);
        column->set_background_color(theme::color::Background2);

        const std::string notice = recomp::mods::conversion_choice_notice();
        if (!notice.empty()) {
            Label *notice_label = context.create_element<Label>(column, notice, theme::Typography::Body);
            notice_label->set_color(theme::color::Primary);
        }

        list_entries();
        Label *hint = context.create_element<Label>(column,
            "A conversion is a ROM hack, cheat codes, saves and textures that play together, with saves of its own. The "
            "choice applies from the next start. AKI Launcher's Play button picks its own.", theme::Typography::Body);
        hint->set_color(theme::color::TextDim);
        status_label = context.create_element<Label>(column, "", theme::Typography::LabelMD);

        Element *list = context.create_element<Element>(column, 0, "div", false);
        list->set_display(Display::Flex);
        list->set_flex_direction(FlexDirection::Column);
        list->set_width(100.0f, Unit::Percent);
        list->set_max_height(560.0f);
        list->set_overflow_y(Overflow::Auto);
        list->set_gap(12.0f);
        list->set_padding(4.0f);
        list->set_as_navigation_container(NavigationType::Vertical);

        const std::vector<menu::Row> rows = menu::rows(shown_entries, recomp::mods::chosen_conversion(),
            recomp::mods::session_conversion_id(), recomp::mods::session_conversion_from_launcher());
        Element *first = nullptr;
        Element *off_button = nullptr;
        for (size_t i = 0; i < rows.size(); i++) {
            const menu::Row &row = rows[i];
            RowWidgets widgets;
            widgets.id = row.entry.id;

            Container *line = context.create_element<Container>(list, FlexDirection::Row, JustifyContent::FlexStart);
            line->set_align_items(AlignItems::Center);
            line->set_gap(16.0f);
            line->set_flex_shrink(0.0f);
            line->set_as_navigation_container(NavigationType::Horizontal);

            // The conversion's icon.png, as a texture of its own (released on refill / close); none for Off.
            Image *image = context.create_element<Image>(line, "");
            image->set_width(64.0f);
            image->set_height(64.0f);
            image->set_min_width(64.0f);
            image->set_background_color(theme::color::BGOverlay);
            if (!row.entry.id.empty() && (i - 1) < shown_folders.size()) {
                const std::string png = read_file(shown_folders[i - 1] / "icon.png");
                if (!png.empty()) {
                    const std::string src = "?/conversions/" + row.entry.id + "/icon/" + std::to_string(icon_generation);
                    recompui::queue_image_from_bytes_file(src, std::vector<char>(png.begin(), png.end()));
                    icons.push_back(src);
                    image->set_src(src);
                }
            }

            Container *text_column = context.create_element<Container>(line, FlexDirection::Column, JustifyContent::Center);
            text_column->set_flex_grow(1.0f);
            text_column->set_gap(4.0f);
            context.create_element<Label>(text_column, row.entry.id.empty() ? std::string("Off (the normal game)") : row.entry.name,
                theme::Typography::LabelMD);
            const std::string detail = row.entry.id.empty() ? std::string("Your own saves and mods") : menu::components_summary(row.entry);
            if (!detail.empty()) {
                Label *detail_label = context.create_element<Label>(text_column, detail, theme::Typography::LabelSM);
                detail_label->set_color(theme::color::TextDim);
            }
            widgets.note = context.create_element<Label>(text_column, row.note, theme::Typography::LabelSM);
            widgets.note->set_color(theme::color::Primary);
            widgets.note->set_display(row.note.empty() ? Display::None : Display::Block);

            widgets.choose = context.create_element<Button>(line, row.chosen ? "Chosen" : "Choose",
                row.chosen ? ButtonStyle::Primary : ButtonStyle::Tertiary, ButtonSize::Small);
            widgets.choose->set_white_space(WhiteSpace::Nowrap);
            widgets.choose->set_flex_shrink(0.0f);
            widgets.choose->set_enabled(row.selectable);
            const std::string id = row.entry.id;
            widgets.choose->add_pressed_callback([id]() { choose(id); });
            // Focus the current choice, unless it can't be played (its button is disabled): then Off.
            if (first == nullptr && row.chosen && row.selectable) {
                first = widgets.choose;
            }
            if (row.entry.id.empty()) {
                off_button = widgets.choose;
            }
            row_widgets.push_back(widgets);
        }

        show_rows();
        if (first == nullptr) {
            first = off_button;
        }
        return first != nullptr ? first : close_button;
    }

    void open_conversions_modal() {
        ContextId prev_context = try_close_current_context();
        return_focus = nullptr;
        if (prev_context != ContextId::null()) {
            // get_focused_element needs its context open.
            prev_context.open();
            return_focus = prev_context.get_focused_element();
            prev_context.close();
        }

        const bool created = (conversions_modal == nullptr);
        if (created) {
            conversions_modal = Modal::create_modal(ModalType::Prompt);
            conversions_modal->set_on_close_callback([]() {
                release_icons();
                row_widgets.clear();
                status_label = nullptr;
                if (return_focus != nullptr) {
                    return_focus->focus();
                    return_focus = nullptr;
                }
            });
        }

        ContextId modal_context = conversions_modal->modal_root_context;
        modal_context.open();
        if (created) {
            // Renders the menu action hints, so it needs the modal's context open.
            conversions_modal->set_menu_action_callback(MenuAction::Back, close_conversions_modal, "Back");
        }
        Element *focus = fill_modal(modal_context);
        modal_context.close();

        if (prev_context != ContextId::null()) {
            prev_context.open();
        }
        conversions_modal->open();
        if (focus != nullptr) {
            focus->focus();
        }
    }
}
