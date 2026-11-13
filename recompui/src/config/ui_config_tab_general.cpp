#include "librecomp/config.hpp"
#include "recompui/config.h"
#include "recompui/recompui.h"
#include "util/steam_deck.h"
#include "composites/ui_save_backups_modal.h"
#include "dd_metrics_rules.h"

#include <cstdio>
#include <cstdlib>

namespace recompui {

static bool created_general_config = false;

namespace config {
    recomp::config::Config &get_general_config() {
        if (!created_general_config) {
            throw std::runtime_error("General config has not been created yet. Call create_general_tab() first.");
        }
        return config::get_config(config::general::id);
    }

    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
    enum class BackgroundInputMode {
        Off,
        On,
        OptionCount
    };
    static EnumOptionVector background_input_options = {
        {BackgroundInputMode::Off, "Off"},
        {BackgroundInputMode::On, "On"},
    };

    template <typename T = uint32_t>
    T get_general_config_enum_value(const std::string& option_id) {
        return static_cast<T>(std::get<uint32_t>(get_general_config().get_option_value(option_id)));
    }

    template <typename T = uint32_t>
    T get_general_config_number_value(const std::string& option_id) {
        return static_cast<T>(std::get<double>(get_general_config().get_option_value(option_id)));
    }

    bool get_general_config_bool_value(const std::string& option_id) {
        return std::get<bool>(get_general_config().get_option_value(option_id));
    }
    
    bool general::has_rumble_strength_option() {
        return get_general_config().has_option(general::options::rumble_strength);
    }

    double general::get_rumble_strength() {
        return get_general_config_number_value<double>(general::options::rumble_strength);
    }

    bool general::has_gyro_sensitivity_option() {
        return get_general_config().has_option(general::options::gyro_sensitivity);
    }
    
    double general::get_gyro_sensitivity() {
        return get_general_config_number_value<double>(general::options::gyro_sensitivity);
    }
    
    bool general::has_mouse_sensitivity_option() {
        return get_general_config().has_option(general::options::mouse_sensitivity);
    }
    
    double general::get_mouse_sensitivity() {
        return get_general_config_number_value<double>(general::options::mouse_sensitivity);
    }
    
    double general::get_joystick_deadzone() {
        return get_general_config_number_value<double>(general::options::joystick_deadzone);
    }
    
    bool general::get_background_input_mode_enabled() {
        return get_general_config_enum_value<BackgroundInputMode>(general::options::background_input_mode) == BackgroundInputMode::On;
    }
    
    bool general::get_debug_mode_enabled() {
        return get_general_config_bool_value(general::options::debug_mode);
    }

    bool general::get_dd_metrics_enabled() {
        return get_general_config_bool_value(general::options::dd_metrics);
    }

    void general::apply_dd_metrics_env() {
        if (!created_general_config || !get_general_config().has_option(general::options::dd_metrics)) {
            return;
        }

        const char *env = std::getenv("WCW_DD_METRICS");
        const bool setting_on = get_dd_metrics_enabled();
        const char *value = dd_metrics_rules::env_value_to_set(env, setting_on);
        if (value == nullptr) {
            if (setting_on) {
                fprintf(stderr, "[wcw dd] Datadog Metrics setting on, but WCW_DD_METRICS=%s overrides it\n", env);
            }
            return;
        }

#if defined(_WIN32)
        _putenv_s("WCW_DD_METRICS", value);
#else
        setenv("WCW_DD_METRICS", value, 1);
#endif
        fprintf(stderr, "[wcw dd] Datadog Metrics setting on: WCW_DD_METRICS=%s for this session\n", value);
    }

    recomp::config::Config &create_general_tab(const GeneralTabOptions& options, const std::string &name) {
        created_general_config = true;
        recomp::config::Config &config = recompui::config::create_config_tab(name, general::id, false);

        config.add_bool_option(
            general::options::debug_mode,
            "Debug Mode",
            "Enables debugging features.",
            false,
            true // hidden by default
        );

        if (options.has_rumble_strength) {
            config.add_percent_number_option(
                general::options::rumble_strength,
                "Rumble Strength",
                "Controls the strength of rumble when using a controller that supports it. "
                "<b>Setting this to zero will disable rumble.</b>",
                25.0
            );
        }

        if (options.has_gyro_sensitivity) {
            config.add_percent_number_option(
                general::options::gyro_sensitivity,
                "Gyro Sensitivity",
                "Controls the sensitivity of gyro aiming when using items in first person for controllers that support it."
                "<b>Setting this to zero will disable gyro.</b>"
                "<br />"
                "<br />"
                "<b>Note: To recalibrate controller gyro, set the controller down on a still, flat surface for 5 seconds.</b>",
                25.0
            );
        }

        if (options.has_mouse_sensitivity) {
            config.add_percent_number_option(
                general::options::mouse_sensitivity,
                "Mouse Sensitivity",
                "Controls the sensitivity of mouse aiming when using items in first person. <b>Setting this to zero will disable mouse aiming.</b>"
                "<br />"
                "<br />"
                "<b>Note: This option does not allow mouse buttons to activate items. Mouse aiming is intended to be used with inputs that are mapped to mouse movement, such as gyro on Steam Deck.</b>",
                is_steam_deck() ? 50.0 : 0.0 // steam deck overrides default to 50%
            );
        }

        config.add_percent_number_option(
            general::options::joystick_deadzone,
            "Joystick Deadzone",
            "Applies a deadzone to joystick inputs.",
            5.0
        );

        config.add_enum_option(
            general::options::background_input_mode,
            "Background Input Mode",
            "Allows the game to read controller input when out of focus."
            "<br/>"
            "<b>This setting does not affect keyboard input.</b>",
            background_input_options,
            BackgroundInputMode::On
        );

        // [wcw dd] Developer option (aki-roadmap#148): read once at startup (config::finalize sets WCW_DD_METRICS).
        config.add_bool_option(
            general::options::dd_metrics,
            "Datadog Metrics",
            "Developer option. Sends performance numbers, and a last message if the game stops on a fatal error, "
            "to a Datadog Agent running on this PC (127.0.0.1), which forwards them to its Datadog account. "
            "With no Agent running, nothing is sent anywhere."
            "<br/>"
            "<b>Takes effect the next time the game starts.</b> The WCW_DD_METRICS environment variable, when set, "
            "overrides this setting.",
            false
        );

        // [wcw saves] Restore a backup (aki-roadmap#50): a button that opens the backup list, not a setting.
        add_config_tab_action(general::id, ConfigTabAction{
            .name = "Restore Save Backup",
            .description = "Your saves are backed up automatically when the game starts and while you play. "
                "Pick a backup by its date to restore it the next time the game starts.",
            .button_label = "Restore a backup...",
            .on_pressed = open_save_backups_modal,
        });

        return config;
    }
} // namespace config
} // namespace recompui
