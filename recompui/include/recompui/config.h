#ifndef _RECOMP_UI_CONFIG_H_
#define _RECOMP_UI_CONFIG_H_

#include <vector>

#include "librecomp/config.hpp"
#include "elements/ui_modal.h"

namespace recompui {
    namespace config {
        namespace general {
            inline const std::string id = "general";
            inline const std::string tab_name = "General";

            namespace options {
                inline const std::string rumble_strength = "rumble_strength";
                inline const std::string gyro_sensitivity = "gyro_sensitivity";
                inline const std::string mouse_sensitivity = "mouse_sensitivity";
                inline const std::string joystick_deadzone = "joystick_deadzone";
                inline const std::string background_input_mode = "background_input_mode";

                inline const std::string debug_mode = "debug_mode";
                // [wcw dd] Sends the in-game Datadog metrics (WCW_DD_METRICS, aki-roadmap#148).
                inline const std::string dd_metrics = "dd_metrics";
            }

            bool has_rumble_strength_option();
            double get_rumble_strength();
            bool has_gyro_sensitivity_option();
            double get_gyro_sensitivity();
            bool has_mouse_sensitivity_option();
            double get_mouse_sensitivity();
            double get_joystick_deadzone();
            bool get_background_input_mode_enabled();
            bool get_debug_mode_enabled();
            bool get_dd_metrics_enabled();
            // [wcw dd] Sets WCW_DD_METRICS for this process from the Datadog Metrics setting, unless the variable is
            // already set (config::finalize calls it, before rt64 starts). Does nothing without a General tab.
            void apply_dd_metrics_env();
        }

        namespace controls {
            inline const std::string id = "controls";
            inline const std::string tab_name = "Controls";

            void add_game_input(
                const std::string &name,
                const std::string &description,
                recompinput::GameInput input_id,
                bool clearable
            );
        }

        namespace graphics {
            inline const std::string id = "graphics";
            inline const std::string tab_name = "Graphics";
    
            namespace options {
                inline const std::string developer_mode = "developer_mode";
                // [wcw] Graphics presets (src/config/graphics_presets.h).
                inline const std::string preset = "graphics_preset";
                inline const std::string res_option = "res_option";
                inline const std::string wm_option = "wm_option";
                inline const std::string hr_option = "hr_option";
                inline const std::string api_option = "api_option";
                inline const std::string ar_option = "ar_option";
                inline const std::string msaa_option = "msaa_option";
                inline const std::string rr_option = "rr_option";
                inline const std::string hpfb_option = "hpfb_option";
                inline const std::string rr_manual_value = "rr_manual_value";
                inline const std::string ds_option = "ds_option";
                // [wcw] Supersampling at Auto resolution (Off / 2x).
                inline const std::string ss_option = "ss_option";
                // [wcw rt] Experimental hybrid ray tracing (RT64 WcwRaytracer).
                // [wcw] Sharp bilinear for point-sampled 2D art (RT64 WcwTextureEdges).
                inline const std::string texture_edges_option = "texture_edges_option";
                inline const std::string rt_option = "rt_option";
                inline const std::string rt_quality_option = "rt_quality_option";
                // [wcw crt] CRT presentation filter (RT64 CrtFilter).
                inline const std::string crt_filter_option = "crt_filter_option";
                inline const std::string crt_signal_option = "crt_signal_option";
                inline const std::string crt_colors_option = "crt_colors_option";
                inline const std::string crt_beam_option = "crt_beam_option";
                // [wcw hdr] HDR output (RT64 HdrOutput).
                inline const std::string hdr_option = "hdr_option";
                inline const std::string hdr_picture = "hdr_picture";
                inline const std::string hdr_custom_nits = "hdr_custom_nits";
            }
    
            void update_msaa_supported(bool supported);
            void update_refresh_rate(uint32_t refresh_rate);
            void toggle_fullscreen();
            // [wcw hdr] Hands the saved HDR settings to RT64; called before the renderer creates its swap chain.
            void apply_hdr_settings();
            // [wcw hdr] Shows in the HDR option whether HDR output actually started, and in HDR Picture the
            // display's figures and what the choice gives.
            void update_hdr_status(bool requested, bool active);
        }

        namespace sound {
            inline const std::string id = "sound";
            inline const std::string tab_name = "Sound";

            namespace options {
                inline const std::string main_volume = "main_volume";
            }

            double get_main_volume();
        }

        namespace mods {
            inline const std::string id = "mods";
            inline const std::string tab_name = "Mods";
        }

        /**
         * @brief Adds a new tab to the config modal, and creates and returns a `Config` for you to add options to.
         * 
         * Once `recompui::config::finalize()` is called, the config will be loaded from disk (`<id>.json`), or this json will be created from the default values.
         * 
         * @param name  The name of the config + tab.
         * @param id  The ID of the config + tab. Also used as the filename for saving/loading (e.g. `<id>.json`).
         * @param requires_confirmation  Whether the tab requires confirmation before closing. Adds an Apply button at the footer, and if the user tries to switch tabs or close the modal, they will be prompted to confirm or cancel changes.
         */
        recomp::config::Config &create_config_tab(const std::string &name, const std::string &id, bool requires_confirmation);

        /**
         * @brief [wcw] A button row at the bottom of a tab made with create_config_tab, for something the player does
         * rather than sets (General > Restore a Backup). It shows a name and a button like an option's, and its
         * description in the tab's description panel.
         */
        struct ConfigTabAction {
            std::string name;
            std::string description;
            std::string button_label;
            std::function<void()> on_pressed;
        };

        /**
         * @brief [wcw] Adds an action to the tab of the config with this ID (after its options, in the order added).
         */
        void add_config_tab_action(const std::string &config_id, ConfigTabAction action);

        /**
         * @brief Adds a new tab to the config modal. This is for manually rendering your own tab's UI.
         * 
         * @param name The name of the tab.
         * @param id The ID of the tab. Can be used to hide/show the tab later.
         * @param create_contents The function to create the body of the tab.
         * @param can_close Gets called when the user tries to either close the modal or switch to a different tab.
         * @param on_close The function to call when the tab is closed.
         */
        void create_tab(
            const std::string &name,
            const std::string &id,
            tab_callbacks::create_contents_t create_contents,
            tab_callbacks::can_close_t can_close = nullptr,
            tab_callbacks::on_close_t on_close = nullptr
        );

        struct GeneralTabOptions {
            bool has_rumble_strength = true;
            bool has_gyro_sensitivity = false;
            bool has_mouse_sensitivity = false;
        };

        // Prefab config tabs.
        // TODO: Explain how to hide/show options
        recomp::config::Config &create_general_tab(const GeneralTabOptions& options, const std::string &name = config::general::tab_name);
        void create_controls_tab(const std::string &name = config::controls::tab_name);
        // [wcw fix] Writes controls.json when the mappings changed since the last save (aki-roadmap#303); the UI calls it every frame.
        void save_controls_if_changed();
        recomp::config::Config &create_graphics_tab(const std::string &name = config::graphics::tab_name);
        recomp::config::Config &create_sound_tab(const std::string &name = config::sound::tab_name);
        void create_mods_tab(const std::string &name = config::mods::tab_name);

        // Must be called after all tabs have been created.
        // This loads all of the user's config files from disk.
        void finalize();

        // Sets a tab with the given ID to be hidden or visible. Can be used for debug tabs, or hidden configs.
        void set_tab_visible(const std::string &id, bool is_visible);

        void set_tab(const std::string &id);
        void open();
        bool close();

        // Get the config modal instance. nullptr until created with init_modal() internally.
        // Will throw an error if it hasn't been created yet.
        recompui::TabbedModal *get_config_modal();

        ContextId get_config_context_id();

        // Get any added config by ID.
        recomp::config::Config &get_config(const std::string &id);
        recomp::config::Config &get_general_config();
        recomp::config::Config &get_graphics_config();
        recomp::config::Config &get_sound_config();

        // Internal use only.
        void init_modal();
    }
}

#endif // _RECOMP_UI_CONFIG_H_
