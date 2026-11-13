#include "graphics_presets.h"

#include <algorithm>

namespace recompui::config::presets {
    using ultramodern::renderer::Antialiasing;
    using ultramodern::renderer::AspectRatio;
    using ultramodern::renderer::RefreshRate;

    static Antialiasing capped_msaa(Antialiasing msaa, const Limits &limits) {
        return std::min(msaa, limits.max_msaa);
    }

    std::optional<Settings> settings_for(Preset preset, const Limits &limits) {
        const bool smooth = limits.smooth_rate > 0;
        Settings settings;
        switch (preset) {
        case Preset::Authentic:
            // The game as it shipped, on a period TV.
            settings.aspect_ratio = AspectRatio::Original;
            settings.framerate = RefreshRate::Original;
            settings.msaa = Antialiasing::None;
            settings.crt = CrtFilterOption::On;
            settings.crt_signal = CrtSignalOption::Composite;
            break;
        case Preset::Enhanced:
        case Preset::EnhancedRayTracing:
            // The default. Enhanced Ray Tracing is the same with ray tracing on.
            settings.aspect_ratio = AspectRatio::Expand;
            settings.framerate = smooth ? RefreshRate::Manual : RefreshRate::Original;
            settings.msaa = Antialiasing::MSAA4X;
            settings.rt = (preset == Preset::EnhancedRayTracing) ? RayTracingOption::On : RayTracingOption::Off;
            settings.rt_quality = RayTracingQuality::Balanced;
            break;
        case Preset::Max:
            settings.aspect_ratio = AspectRatio::Expand;
            // The smooth rate, never twice it (aki-roadmap#165): ray tracing at Ultra and 120 fps falls to ~7 frames
            // a second, so Max on a 120 Hz+ display was a slideshow. 120 fps stays in the Framerate menu.
            settings.framerate = smooth ? RefreshRate::Manual : RefreshRate::Original;
            settings.msaa = Antialiasing::MSAA4X;
            settings.rt = RayTracingOption::On;
            settings.rt_quality = RayTracingQuality::Ultra;
            settings.hdr = HdrOption::On;
            settings.hdr_picture = HdrPictureOption::Auto;
            break;
        case Preset::Custom:
        default:
            return std::nullopt;
        }

        settings.msaa = capped_msaa(settings.msaa, limits);
        return settings;
    }

    bool matches(Preset preset, const Settings &current, const Limits &limits) {
        const std::optional<Settings> wanted = settings_for(preset, limits);
        if (!wanted) {
            return false;
        }

        return (current.aspect_ratio == wanted->aspect_ratio) &&
            ((limits.smooth_rate == 0) || (current.framerate == wanted->framerate)) &&
            (capped_msaa(current.msaa, limits) == wanted->msaa) &&
            (current.crt == wanted->crt) &&
            ((current.crt == CrtFilterOption::Off) || (current.crt_signal == wanted->crt_signal)) &&
            (current.rt == wanted->rt) &&
            ((current.rt == RayTracingOption::Off) || (current.rt_quality == wanted->rt_quality)) &&
            (current.hdr == wanted->hdr) &&
            ((current.hdr == HdrOption::Off) || (current.hdr_picture == wanted->hdr_picture));
    }

    Preset matching_preset(const Settings &current, const Limits &limits) {
        // Enhanced and Enhanced Ray Tracing differ only in ray tracing, so at most one of them matches.
        for (Preset preset : { Preset::Authentic, Preset::Enhanced, Preset::EnhancedRayTracing, Preset::Max }) {
            if (matches(preset, current, limits)) {
                return preset;
            }
        }

        return Preset::Custom;
    }

    const char *label(Preset preset) {
        switch (preset) {
        case Preset::Authentic:
            return "Authentic";
        case Preset::Enhanced:
            return "Enhanced";
        case Preset::EnhancedRayTracing:
            return "Enhanced RT";
        case Preset::Max:
            return "Max";
        case Preset::Custom:
        default:
            return "Custom";
        }
    }
};
