#pragma once

#include <span>

// disable warning: '/*' within block comment [-Wcomment] for GamepadMotion.hpp
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomment"
#include "GamepadMotion.hpp"
#pragma GCC diagnostic pop

#include "ultramodern/input.hpp"
#include "recompinput/recompinput.h"

namespace recompinput {
    struct ControllerState {
        SDL_GameController* controller;
        std::array<float, 3> latest_accelerometer;
        GamepadMotion motion;
        uint32_t prev_gyro_timestamp;
        std::array<float, 2> rotation_delta{};
        std::array<float, 2> pending_rotation_delta{};
        bool rumble_failed;

        ControllerState() : controller{}, latest_accelerometer{}, motion{}, prev_gyro_timestamp{}, rotation_delta{}, pending_rotation_delta{}, rumble_failed{} {
            motion.Reset();
            motion.SetCalibrationMode(GamepadMotionHelpers::CalibrationMode::Stillness | GamepadMotionHelpers::CalibrationMode::SensorFusion);
        };
    };

    void poll_inputs();
    void set_rumble(int controller_num, bool on);
    void update_rumble();
    bool should_override_keystate(SDL_Scancode key, SDL_Keymod mod);
    float get_input_analog(int controller_num, const InputField& field);
    float get_input_analog(int controller_num, const std::span<const InputField> fields);
    bool get_input_digital(int controller_num, const InputField& field);
    bool get_input_digital(int controller_num, const std::span<const InputField> fields);
    void get_gyro_deltas(int controller_num, float* x, float* y);
    void get_mouse_deltas(float* x, float* y);
    void get_right_analog(int controller_num, float* x, float* y);
    void apply_joystick_deadzone(float x_in, float y_in, float* x_out, float* y_out);
    void set_right_analog_suppressed(bool suppressed);
    // [wcw freecam] For mods that take over the keyboard and mouse (AkiMods camera):
    // keyboard bindings stop reaching the game; the cursor is locked to the window (relative
    // mouse) while game input is enabled; raw mouse motion and wheel, unscaled and only while
    // game input is enabled, are accumulated until taken.
    void set_keyboard_suppressed(bool suppressed);
    void set_mouse_capture_requested(bool requested);
    bool mouse_capture_requested();
    void take_raw_mouse_motion(float* delta_x, float* delta_y, int32_t* wheel);
    bool game_input_disabled();
    bool all_input_disabled();
    // [wcw fix] Forgets which sticks/triggers bound to buttons were held, for one player (aki-roadmap#305): called while
    // the game gets no input (a menu is open), so a stick resting between the release and press thresholds afterwards
    // doesn't read as still held.
    void reset_analog_digital_state(int controller_num);
    ControllerState &get_controller_state(SDL_JoystickID joystick_id);
    SDL_GameController *get_controller_from_joystick_id(SDL_JoystickID joystick_id);
    void add_controller_state(SDL_JoystickID joystick_id, SDL_GameController* controller);
    void remove_controller_state(SDL_JoystickID joystick_id);
    void add_rotation_deltas(SDL_JoystickID joystick_id, float rot_x, float rot_y);
    void add_mouse_deltas(float delta_x, float delta_y);
    void add_mouse_wheel_delta(int32_t delta);
}
