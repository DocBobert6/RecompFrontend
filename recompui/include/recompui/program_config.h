#pragma once

#include "recompui.h"
#include <cstdint>
#include <string>

namespace recompui {
    namespace programconfig {
        void set_program_name(const std::string& name);
        void set_program_id(const std::u8string& id);
        const std::string &get_program_name();
        const std::u8string &get_program_id();

        // [wcw hfr] Frame interpolation the game supports. smooth_rate = 0 (the default) means none: the
        // Framerate option stays locked to Original. Otherwise it offers "30 (Original)" and
        // "<smooth_rate> (Smooth)"; the remaining fields tune RT64's interpolation for the game's engine.
        // Call before the config tabs are created (next to set_program_id).
        struct FrameInterpolation {
            uint32_t smooth_rate = 0;
            // Derive camera-independent object matrices from the camera hand-off (no game-side hooks).
            bool hookless_objects = false;
            // Start generated frames from the live image (engines that clear buffers a frame early).
            bool prime_generated_frames = false;
            // Never interpolate into a framebuffer that is only cleared (engines that pre-clear the next buffer).
            bool skip_clear_only_targets = true;
            // Spread generated frames evenly over the game frame.
            bool even_pacing = true;
        };
        void set_frame_interpolation(const FrameInterpolation &interpolation);
        const FrameInterpolation &get_frame_interpolation();
    }
}
