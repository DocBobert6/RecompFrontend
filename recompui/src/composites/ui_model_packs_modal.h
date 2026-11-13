#pragma once

namespace recompui {
    // [wcw model import] Mods > aki_modelimport > Model packs (aki-roadmap#130): a dialog listing the model packs in
    // <mods>/models (preview, name, author) with Off / Slot 1-4 for each; a choice rewrites <mods>/models/models.txt,
    // which the mod reads again at the start of every match. Rules: config/model_pack_menu.h. Only reachable from the
    // aki_modelimport mod's details panel, so it does nothing in a game without that mod.
    void open_model_packs_modal();

    // The mod id whose details panel offers the dialog.
    constexpr const char *model_packs_mod_id = "aki_modelimport";
}
