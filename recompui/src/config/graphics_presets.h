#pragma once

#include <cstdint>
#include <optional>

#include "ultramodern/config.hpp"

namespace recompui::config {
    // [wcw rt] Experimental hybrid ray tracing.
    enum class RayTracingOption {
        Off = 0,
        On = 1
    };

    // Matches RT64::WcwLogic::RtQuality. Ambient occlusion comes with the level (none on Performance, Subtle on
    // Balanced, Strong on Quality and Ultra), so there is no separate AO option.
    enum class RayTracingQuality {
        Performance = 0,
        Balanced = 1,
        Quality = 2,
        Ultra = 3
    };

    // [wcw crt] CRT filter: on/off, the video signal (a preset in crt/presets/ next to the
    // executable) and the TV's color standard.
    enum class CrtFilterOption {
        Off = 0,
        On = 1
    };

    enum class CrtSignalOption {
        Composite = 0,
        SVideo = 1,
        RF = 2,
        RGB = 3
    };

    // [wcw hdr] HDR output.
    enum class HdrOption {
        Off = 0,
        On = 1
    };

    // Same values as RT64::WcwLogic::HdrPicture.
    enum class HdrPictureOption {
        Auto = 0,
        Brighter = 1,
        Balanced = 2,
        CrtDetail = 3,
        Custom = 4
    };
};

// [wcw] Graphics presets (Graphics > Preset): what each preset sets, and which preset a set of options matches. Kept
// free of UI state so it can be unit tested (recompui/tests); the graphics tab calls these. A change to a preset
// belongs here, with a test.
namespace recompui::config::presets {
    // Stored values of the Preset option. Custom is never picked: the menu shows it when the options match no preset.
    enum class Preset {
        Authentic = 0,
        Enhanced = 1,
        EnhancedRayTracing = 2,
        Max = 3,
        Custom = 4
    };

    // The options a preset sets. The CRT signal only counts with the CRT filter on, the ray tracing quality only with
    // ray tracing on and the HDR picture only with HDR on (the menu hides them otherwise).
    struct Settings {
        ultramodern::renderer::AspectRatio aspect_ratio = ultramodern::renderer::AspectRatio::Expand;
        // The Framerate menu's stored choice (framerate_rules.h): Original, Manual = Smooth, Display = Smooth2x.
        ultramodern::renderer::RefreshRate framerate = ultramodern::renderer::RefreshRate::Original;
        ultramodern::renderer::Antialiasing msaa = ultramodern::renderer::Antialiasing::None;
        CrtFilterOption crt = CrtFilterOption::Off;
        CrtSignalOption crt_signal = CrtSignalOption::Composite;
        RayTracingOption rt = RayTracingOption::Off;
        RayTracingQuality rt_quality = RayTracingQuality::Balanced;
        HdrOption hdr = HdrOption::Off;
        HdrPictureOption hdr_picture = HdrPictureOption::Auto;
    };

    // What the game and the machine allow.
    struct Limits {
        // The game's smooth rate (programconfig::get_frame_interpolation); 0 = the framerate is locked to Original.
        uint32_t smooth_rate = 0;
        // The display's refresh rate in Hz; 0 = not known yet. No preset uses it since Max stopped picking 120 fps
        // (aki-roadmap#165); kept for presets that might again.
        uint32_t display_rate = 0;
        // The highest MSAA level the GPU offers.
        ultramodern::renderer::Antialiasing max_msaa = ultramodern::renderer::Antialiasing::MSAA4X;
    };

    // The settings a preset applies on this machine; nothing for Custom.
    std::optional<Settings> settings_for(Preset preset, const Limits &limits);

    // Whether the options are what the preset sets. MSAA is compared as the GPU runs it (capped at max_msaa) and the
    // framerate is ignored when the game locks it.
    bool matches(Preset preset, const Settings &current, const Limits &limits);

    // The first preset the options match, or Custom.
    Preset matching_preset(const Settings &current, const Limits &limits);

    // The preset's name in the menu, short enough for all five to fit on one row: Authentic / Enhanced / Enhanced RT /
    // Max / Custom.
    const char *label(Preset preset);
};
