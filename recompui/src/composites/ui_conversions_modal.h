#pragma once

namespace recompui {
    // [wcw fullconv] Mods > Conversions (aki-roadmap#57): a dialog listing the conversions installed in <config>/conversions
    // (icon, name, components) with Off and one choice; a choice writes conversions/active.txt through librecomp's
    // conversion picker API and applies from the next start. Rules: config/conversion_menu.h.
    void open_conversions_modal();
}
