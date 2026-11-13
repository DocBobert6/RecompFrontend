#ifndef __RECOMP_INPUT_PROFILES_H__
#define __RECOMP_INPUT_PROFILES_H__

#include <filesystem>
#include <string>
#include "recompinput.h"
#include "json/json.hpp"
#include "controls_rules.h"


namespace recompinput {
    // ControllerGUID and its json conversion live in controls_rules.h (unit tested without SDL).

    namespace profiles {
        constexpr int controls_config_version = 3;

        recompinput::InputField& get_input_binding(int profile_index, GameInput input, size_t binding_index);
        void set_input_binding(int profile_index, recompinput::GameInput input, size_t binding_index, recompinput::InputField value);
        void clear_input_binding(int profile_index, GameInput input);
        void reset_input_binding(int profile_index, InputDevice device, GameInput input);
        void reset_profile_bindings(int profile_index, InputDevice device);
        int add_input_profile(const std::string &key, const std::string &name, InputDevice device, bool custom);
        int get_input_profile_by_key(const std::string &key);
        const std::string &get_input_profile_key(int profile_index);
        const std::string &get_input_profile_name(int profile_index);
        InputDevice get_input_profile_device(int profile_index);
        bool is_input_profile_custom(int profile_index);
        int get_input_profile_count();
        const std::vector<int> get_indices_for_custom_profiles(InputDevice device);
        void set_input_profile_for_player(int player_index, int profile_index, InputDevice device);
        int get_input_profile_for_player(int player_index, InputDevice device);
        int add_controller(ControllerGUID guid, int profile_index);
        const ControllerGUID &get_controller_guid(int controller_index);
        int get_controller_profile_index(int controller_index);
        int get_controller_by_guid(ControllerGUID guid);
        int get_controller_count();
        ControllerGUID get_guid_from_sdl_controller(SDL_GameController *game_controller);
        int get_controller_profile_index_from_sdl_controller(SDL_GameController *game_controller);
        std::string get_mp_keyboard_profile_key(int player_index);
        std::string get_mp_keyboard_profile_name(int player_index);
        void initialize_input_bindings();
        int get_sp_controller_profile_index();
        int get_sp_keyboard_profile_index();
        int get_or_create_mp_keyboard_profile_index(int player_index);
        std::string get_string_from_controller_guid(ControllerGUID guid);
        bool get_n64_input(int player_index, uint16_t* buttons_out, float* x_out, float* y_out);

        bool load_controls_config(const std::filesystem::path& path);
        bool save_controls_config(const std::filesystem::path& path);
        // [wcw fix] Saves the controls when they changed since the last load or save (aki-roadmap#303). Called every UI
        // frame; does nothing before load_controls_config or while binding_active.
        bool save_controls_config_if_changed(const std::filesystem::path& path, bool binding_active);
    }
}

#endif // __RECOMP_INPUT_PROFILES_H__
