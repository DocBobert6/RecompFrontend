#include "ui_model_packs_modal.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

#include "librecomp/mods.hpp"
#include "recompui/recompui.h"
#include "elements/ui_modal.h"
#include "elements/ui_label.h"
#include "elements/ui_image.h"
#include "elements/ui_container.h"
#include "config/ui_config_option.h"
#include "config/model_pack_menu.h"

namespace recompui {
    namespace menu = config::model_pack_menu;

    static Modal *packs_modal = nullptr;
    // What had focus before the dialog opened (the Model packs button), to give it back on close.
    static Element *return_focus = nullptr;
    // Preview textures queued for the rows shown now, released when the dialog refills or closes.
    static std::vector<std::string> previews;
    static unsigned preview_generation = 0;   // in the texture names: the image cache never replaces a name
    // The rows shown now. A choice updates them in place: rebuilding the list from a button's own callback would
    // destroy that button while it runs.
    struct RowWidgets {
        std::string id;
        Button *choice[menu::slot_count + 1] = {};
        Label *note = nullptr;
    };
    static std::vector<RowWidgets> row_widgets;
    static std::vector<menu::Pack> shown_packs;

    static std::filesystem::path models_directory() {
        return recomp::mods::get_mods_directory() / "models";
    }

    static std::string read_text(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    static bool write_text(const std::filesystem::path &path, const std::string &text) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        const std::filesystem::path tmp = path.string() + ".new";
        {
            std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
            if (!file) {
                return false;
            }
            file << text;
            file.close();   // the buffered data is written here: check it before replacing the file
            if (file.fail()) {
                std::filesystem::remove(tmp, ec);
                return false;
            }
        }
        std::filesystem::rename(tmp, path, ec);   // replaces it (the mod reads it only at a match start)
        return !ec;
    }

    // The packs on disk: <mods>/models/<id>.akimodel folders with a valid id, with their pack.json fields.
    static std::vector<menu::Pack> list_packs() {
        std::vector<menu::Pack> packs;
        std::error_code ec;
        const std::string ext = ".akimodel";
        for (const auto &entry : std::filesystem::directory_iterator(models_directory(), ec)) {
            std::error_code dir_ec;
            if (!entry.is_directory(dir_ec)) {
                continue;
            }
            const std::wstring wide = entry.path().filename().wstring();
            if (std::any_of(wide.begin(), wide.end(), [](wchar_t c) { return c < 0x20 || c > 0x7E; })) {
                continue;   // not a pack id (ASCII only), and converting it could throw
            }
            const std::string name(wide.begin(), wide.end());
            if (name.size() <= ext.size() || name.compare(name.size() - ext.size(), ext.size(), ext) != 0) {
                continue;
            }
            const std::string id = name.substr(0, name.size() - ext.size());
            if (!menu::valid_pack_id(id)) {
                continue;
            }
            packs.push_back(menu::parse_pack_json(id, read_text(entry.path() / "pack.json")));
        }
        return packs;
    }

    static void release_previews() {
        for (const std::string &src : previews) {
            recompui::release_image(src);
        }
        previews.clear();
    }

    static void close_model_packs_modal() {
        if (packs_modal != nullptr && packs_modal->is_open_now()) {
            packs_modal->close();
        }
    }

    // Shows text's assignments on the rows: the current choice highlighted, the note.
    static void show_rows(const std::string &text) {
        const std::vector<menu::Row> rows = menu::rows(shown_packs, text);
        for (RowWidgets &w : row_widgets) {
            for (const menu::Row &row : rows) {
                if (row.pack.id != w.id) {
                    continue;
                }
                for (int slot = 0; slot <= menu::slot_count; slot++) {
                    w.choice[slot]->apply_button_style(row.slot == slot ? ButtonStyle::Primary : ButtonStyle::Tertiary);
                }
                w.note->set_text(row.note);
                w.note->set_display(row.note.empty() ? Display::None : Display::Block);
            }
        }
    }

    static void choose(const std::string &pack_id, int slot) {
        const std::filesystem::path path = models_directory() / "models.txt";
        const std::string text = menu::assign(read_text(path), pack_id, slot);
        if (!write_text(path, text)) {
            recompui::open_info_prompt("Model packs", "Couldn't write models.txt in the mods\\models folder (check its permissions).",
                "OK", {}, recompui::ButtonStyle::Primary);
            return;
        }
        show_rows(text);
    }

    // Fills the dialog for the packs on disk now. Must be called with the modal's context open; returns the element
    // to focus (the first row's current choice).
    static Element *fill_modal(ContextId context) {
        release_previews();
        preview_generation++;
        row_widgets.clear();

        Element *header_left = packs_modal->get_header()->get_left();
        header_left->clear_children();
        header_left->set_padding(16.0f);
        header_left->set_padding_left(20.0f);
        context.create_element<Label>(header_left, "Model packs", theme::Typography::Header3);

        Element *header_right = packs_modal->get_header()->get_right();
        header_right->clear_children();
        header_right->set_padding(16.0f);
        Button *close_button = context.create_element<Button>(header_right, "Close", ButtonStyle::Secondary, ButtonSize::Medium);
        close_button->add_pressed_callback(close_model_packs_modal);

        Element *body = packs_modal->get_body();
        body->clear_children();

        Element *column = context.create_element<Element>(body, 0, "div", false);
        column->set_display(Display::Flex);
        column->set_flex_direction(FlexDirection::Column);
        column->set_width(100.0f, Unit::Percent);
        column->set_padding(20.0f);
        column->set_padding_top(4.0f);
        column->set_gap(16.0f);
        column->set_as_navigation_container(NavigationType::Vertical);
        // Opaque: the Prompt modal's overlay colour lets the Mods page behind show through a dense list.
        column->set_background_color(theme::color::Background2);

        shown_packs = list_packs();
        if (shown_packs.empty()) {
            context.create_element<Label>(column, "No model packs yet", theme::Typography::LabelMD);
            Label *hint = context.create_element<Label>(column,
                "Put a pack folder (<name>.akimodel, made with AkiTools aki-model.py) in the mods\\models folder.",
                theme::Typography::Body);
            hint->set_color(theme::color::TextDim);
            return close_button;
        }

        Label *hint = context.create_element<Label>(column,
            "Pick the wrestler slot each pack replaces. Changes apply at the start of the next match. A Slot N pack set in "
            "the mod's Configure options takes priority over this list.", theme::Typography::Body);
        hint->set_color(theme::color::TextDim);

        const std::vector<menu::Row> rows = menu::rows(shown_packs, read_text(models_directory() / "models.txt"));

        Element *list = context.create_element<Element>(column, 0, "div", false);
        list->set_display(Display::Flex);
        list->set_flex_direction(FlexDirection::Column);
        list->set_width(100.0f, Unit::Percent);
        list->set_max_height(560.0f);
        list->set_overflow_y(Overflow::Auto);
        list->set_gap(12.0f);
        list->set_padding(4.0f);
        list->set_as_navigation_container(NavigationType::Vertical);

        Element *first = nullptr;
        for (const menu::Row &row : rows) {
            RowWidgets widgets;
            widgets.id = row.pack.id;

            Container *line = context.create_element<Container>(list, FlexDirection::Row, JustifyContent::FlexStart);
            line->set_align_items(AlignItems::Center);
            line->set_gap(16.0f);
            line->set_flex_shrink(0.0f);
            line->set_as_navigation_container(NavigationType::Horizontal);

            // The pack's preview.png, as a texture of its own (released on refill / close).
            Image *image = context.create_element<Image>(line, "");
            image->set_width(96.0f);
            image->set_height(96.0f);
            image->set_min_width(96.0f);
            image->set_background_color(theme::color::BGOverlay);
            const std::string png = read_text(models_directory() / (row.pack.id + ".akimodel") / "preview.png");
            if (!png.empty()) {
                const std::string src = "?/models/" + row.pack.id + "/preview/" + std::to_string(preview_generation);
                recompui::queue_image_from_bytes_file(src, std::vector<char>(png.begin(), png.end()));
                previews.push_back(src);
                image->set_src(src);
            }

            Container *text_column = context.create_element<Container>(line, FlexDirection::Column, JustifyContent::Center);
            text_column->set_flex_grow(1.0f);
            text_column->set_gap(4.0f);
            context.create_element<Label>(text_column, row.pack.name, theme::Typography::LabelMD);
            std::string detail = row.pack.id + ".akimodel";
            if (!row.pack.author.empty()) {
                detail += " - " + row.pack.author;
            }
            Label *detail_label = context.create_element<Label>(text_column, detail, theme::Typography::LabelSM);
            detail_label->set_color(theme::color::TextDim);
            widgets.note = context.create_element<Label>(text_column, row.note, theme::Typography::LabelSM);
            widgets.note->set_color(theme::color::Primary);
            widgets.note->set_display(row.note.empty() ? Display::None : Display::Block);

            Container *choices = context.create_element<Container>(line, FlexDirection::Row, JustifyContent::FlexEnd);
            choices->set_gap(8.0f);
            choices->set_as_navigation_container(NavigationType::Horizontal);
            for (int slot = 0; slot <= menu::slot_count; slot++) {
                const bool current = (row.slot == slot);
                Button *b = context.create_element<Button>(choices, slot == 0 ? std::string("Off") : ("Slot " + std::to_string(slot)),
                    current ? ButtonStyle::Primary : ButtonStyle::Tertiary, ButtonSize::Small);
                b->set_white_space(WhiteSpace::Nowrap);
                b->set_flex_shrink(0.0f);
                const std::string id = row.pack.id;
                b->add_pressed_callback([id, slot]() { choose(id, slot); });
                widgets.choice[slot] = b;
                if (first == nullptr && current) {
                    first = b;
                }
            }
            row_widgets.push_back(widgets);
        }
        return first != nullptr ? first : close_button;
    }

    void open_model_packs_modal() {
        ContextId prev_context = try_close_current_context();
        return_focus = nullptr;
        if (prev_context != ContextId::null()) {
            // get_focused_element needs its context open.
            prev_context.open();
            return_focus = prev_context.get_focused_element();
            prev_context.close();
        }

        const bool created = (packs_modal == nullptr);
        if (created) {
            packs_modal = Modal::create_modal(ModalType::Prompt);
            packs_modal->set_on_close_callback([]() {
                release_previews();
                row_widgets.clear();
                if (return_focus != nullptr) {
                    return_focus->focus();
                    return_focus = nullptr;
                }
            });
        }

        ContextId modal_context = packs_modal->modal_root_context;
        modal_context.open();
        if (created) {
            // Renders the menu action hints, so it needs the modal's context open.
            packs_modal->set_menu_action_callback(MenuAction::Back, close_model_packs_modal, "Back");
        }
        Element *focus = fill_modal(modal_context);
        modal_context.close();

        if (prev_context != ContextId::null()) {
            prev_context.open();
        }
        packs_modal->open();
        if (focus != nullptr) {
            focus->focus();
        }
    }
}
