#include "recompui/config.h"
#include "recompinput/players.h"
#include "recompinput/profiles.h"
#include "recompinput/input_binding.h"
#include "elements/ui_element.h"
#include "ui_config_page_controls.h"
#include "librecomp/game.hpp"

namespace recompui {

void config::create_controls_tab(const std::string &name) {
    config::create_tab(
        name,
        config::controls::id,
        [](ContextId context, Element* parent) {
            context.create_element<ConfigPageControls>(parent);
        },
        nullptr,
        [](TabCloseContext close_context) {
            // Save the current control mappings when closing the controls tab.
            recompinput::profiles::save_controls_config(recomp::get_config_path() / (config::controls::id + ".json"));
        }
    );
}

// [wcw fix] Saves the controls as soon as they change (aki-roadmap#303), not only when the Controls tab closes:
// quitting from the menu or closing the window with the tab open used to lose every change.
void config::save_controls_if_changed() {
    const std::filesystem::path config_path = recomp::get_config_path();
    if (config_path.empty()) {
        return;
    }
    recompinput::profiles::save_controls_config_if_changed(config_path / (config::controls::id + ".json"),
        recompinput::binding::is_binding());
}

} // namespace recompui
