// [wcw] Unit tests for the graphics presets (src/config/graphics_presets). Built and run as part of every recompui
// build (RECOMPUI_WCW_TESTS); a failure fails the build.

#include "graphics_presets.h"

#include "wcw_test.h"

using ultramodern::renderer::Antialiasing;
using ultramodern::renderer::AspectRatio;
using ultramodern::renderer::RefreshRate;
using namespace recompui::config;
using namespace recompui::config::presets;

template <typename T>
static int val(T value) {
    return static_cast<int>(value);
}

// A game with frame interpolation (smooth rate 60) on a 144 Hz display with an MSAA 4x GPU.
static Limits interpolating() {
    Limits limits;
    limits.smooth_rate = 60;
    limits.display_rate = 144;
    limits.max_msaa = Antialiasing::MSAA4X;
    return limits;
}

static Settings preset(Preset p, const Limits &limits = interpolating()) {
    return settings_for(p, limits).value();
}

TEST_CASE("Authentic is the game as it shipped on a composite TV") {
    const Settings s = preset(Preset::Authentic);
    CHECK_EQ(val(s.aspect_ratio), val(AspectRatio::Original));
    CHECK_EQ(val(s.framerate), val(RefreshRate::Original));
    CHECK_EQ(val(s.msaa), val(Antialiasing::None));
    CHECK_EQ(val(s.crt), val(CrtFilterOption::On));
    CHECK_EQ(val(s.crt_signal), val(CrtSignalOption::Composite));
    CHECK_EQ(val(s.rt), val(RayTracingOption::Off));
    CHECK_EQ(val(s.hdr), val(HdrOption::Off));
}

TEST_CASE("Enhanced is Expand, 60 fps (Smooth), MSAA 4x, no ray tracing, CRT or HDR") {
    const Settings s = preset(Preset::Enhanced);
    CHECK_EQ(val(s.aspect_ratio), val(AspectRatio::Expand));
    CHECK_EQ(val(s.framerate), val(RefreshRate::Manual));
    CHECK_EQ(val(s.msaa), val(Antialiasing::MSAA4X));
    CHECK_EQ(val(s.crt), val(CrtFilterOption::Off));
    CHECK_EQ(val(s.rt), val(RayTracingOption::Off));
    CHECK_EQ(val(s.hdr), val(HdrOption::Off));
}

TEST_CASE("Enhanced Ray Tracing is Expand, 60 fps (Smooth), MSAA 4x, RT Balanced, no CRT or HDR") {
    const Settings s = preset(Preset::EnhancedRayTracing);
    CHECK_EQ(val(s.aspect_ratio), val(AspectRatio::Expand));
    CHECK_EQ(val(s.framerate), val(RefreshRate::Manual));
    CHECK_EQ(val(s.msaa), val(Antialiasing::MSAA4X));
    CHECK_EQ(val(s.crt), val(CrtFilterOption::Off));
    CHECK_EQ(val(s.rt), val(RayTracingOption::On));
    CHECK_EQ(val(s.rt_quality), val(RayTracingQuality::Balanced));
    CHECK_EQ(val(s.hdr), val(HdrOption::Off));
}

TEST_CASE("Enhanced and Enhanced Ray Tracing differ only in ray tracing") {
    const Settings plain = preset(Preset::Enhanced);
    Settings s = preset(Preset::EnhancedRayTracing);
    s.rt = RayTracingOption::Off;
    CHECK_EQ(val(s.aspect_ratio), val(plain.aspect_ratio));
    CHECK_EQ(val(s.framerate), val(plain.framerate));
    CHECK_EQ(val(s.msaa), val(plain.msaa));
    CHECK_EQ(val(s.crt), val(plain.crt));
    CHECK_EQ(val(s.hdr), val(plain.hdr));
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Enhanced));
}

TEST_CASE("the default is Enhanced, with ray tracing off") {
    // The tab builds the options' defaults from settings_for(Enhanced) and the Preset option defaults to Enhanced, so a
    // fresh config is these settings.
    const Settings defaults = preset(Preset::Enhanced);
    CHECK_EQ(val(defaults.rt), val(RayTracingOption::Off));
    CHECK_EQ(val(matching_preset(defaults, interpolating())), val(Preset::Enhanced));

    // The default Settings (Expand, Original framerate, no MSAA) match nothing; only Enhanced's do.
    CHECK_EQ(val(matching_preset(Settings{}, interpolating())), val(Preset::Custom));
}

TEST_CASE("the old Enhanced (RT Balanced) now matches Enhanced Ray Tracing") {
    Settings s;
    s.aspect_ratio = AspectRatio::Expand;
    s.framerate = RefreshRate::Manual;
    s.msaa = Antialiasing::MSAA4X;
    s.rt = RayTracingOption::On;
    s.rt_quality = RayTracingQuality::Balanced;
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::EnhancedRayTracing));
    CHECK(!matches(Preset::Enhanced, s, interpolating()));
}

TEST_CASE("stored preset values follow the menu order") {
    CHECK_EQ(val(Preset::Authentic), 0);
    CHECK_EQ(val(Preset::Enhanced), 1);
    CHECK_EQ(val(Preset::EnhancedRayTracing), 2);
    CHECK_EQ(val(Preset::Max), 3);
    CHECK_EQ(val(Preset::Custom), 4);
}

TEST_CASE("Max is 60 fps (Smooth), MSAA 4x, RT Ultra, HDR Auto") {
    const Settings s = preset(Preset::Max);
    CHECK_EQ(val(s.aspect_ratio), val(AspectRatio::Expand));
    CHECK_EQ(val(s.framerate), val(RefreshRate::Manual));
    CHECK_EQ(val(s.msaa), val(Antialiasing::MSAA4X));
    CHECK_EQ(val(s.crt), val(CrtFilterOption::Off));
    CHECK_EQ(val(s.rt), val(RayTracingOption::On));
    CHECK_EQ(val(s.rt_quality), val(RayTracingQuality::Ultra));
    CHECK_EQ(val(s.hdr), val(HdrOption::On));
    CHECK_EQ(val(s.hdr_picture), val(HdrPictureOption::Auto));
}

TEST_CASE("Max never pairs RT Ultra with 120 fps, whatever the display (aki-roadmap#165)") {
    Limits limits = interpolating();
    for (uint32_t rate : { 0u, 60u, 100u, 119u, 120u, 144u, 165u, 240u }) {
        limits.display_rate = rate;
        const Settings s = preset(Preset::Max, limits);
        CHECK_EQ(val(s.rt_quality), val(RayTracingQuality::Ultra));
        CHECK_EQ(val(s.framerate), val(RefreshRate::Manual));
    }
}

TEST_CASE("Max with 120 fps chosen by hand is Custom (aki-roadmap#165)") {
    Limits limits = interpolating();
    limits.display_rate = 165;
    Settings s = preset(Preset::Max, limits);
    CHECK_EQ(val(matching_preset(s, limits)), val(Preset::Max));
    s.framerate = RefreshRate::Display;
    CHECK_EQ(val(matching_preset(s, limits)), val(Preset::Custom));
}

TEST_CASE("a game with a locked framerate stays at Original in every preset") {
    Limits limits = interpolating();
    limits.smooth_rate = 0;
    for (Preset p : { Preset::Authentic, Preset::Enhanced, Preset::EnhancedRayTracing, Preset::Max }) {
        CHECK_EQ(val(preset(p, limits).framerate), val(RefreshRate::Original));
    }
}

TEST_CASE("MSAA is capped at what the GPU offers") {
    Limits limits = interpolating();
    limits.max_msaa = Antialiasing::MSAA2X;
    CHECK_EQ(val(preset(Preset::Enhanced, limits).msaa), val(Antialiasing::MSAA2X));
    CHECK_EQ(val(preset(Preset::EnhancedRayTracing, limits).msaa), val(Antialiasing::MSAA2X));
    CHECK_EQ(val(preset(Preset::Max, limits).msaa), val(Antialiasing::MSAA2X));
    limits.max_msaa = Antialiasing::None;
    CHECK_EQ(val(preset(Preset::Max, limits).msaa), val(Antialiasing::None));
}

TEST_CASE("Custom sets nothing") {
    CHECK(!settings_for(Preset::Custom, interpolating()).has_value());
    CHECK(!matches(Preset::Custom, preset(Preset::Enhanced), interpolating()));
    CHECK(!matches(Preset::Custom, preset(Preset::EnhancedRayTracing), interpolating()));
}

TEST_CASE("each preset's settings match that preset") {
    for (Preset p : { Preset::Authentic, Preset::Enhanced, Preset::EnhancedRayTracing, Preset::Max }) {
        CHECK_EQ(val(matching_preset(preset(p), interpolating())), val(p));
    }

    Limits locked = interpolating();
    locked.smooth_rate = 0;
    locked.max_msaa = Antialiasing::None;
    for (Preset p : { Preset::Authentic, Preset::Enhanced, Preset::EnhancedRayTracing, Preset::Max }) {
        CHECK_EQ(val(matching_preset(preset(p, locked), locked)), val(p));
    }
}

TEST_CASE("changing any one option a preset sets makes it Custom") {
    for (Preset p : { Preset::Authentic, Preset::Enhanced, Preset::EnhancedRayTracing, Preset::Max }) {
        const Settings base = preset(p);
        Settings s;

        s = base;
        s.aspect_ratio = (base.aspect_ratio == AspectRatio::Expand) ? AspectRatio::Original : AspectRatio::Expand;
        CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));

        s = base;
        s.framerate = (base.framerate == RefreshRate::Original) ? RefreshRate::Manual : RefreshRate::Original;
        CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));

        s = base;
        s.msaa = (base.msaa == Antialiasing::MSAA2X) ? Antialiasing::None : Antialiasing::MSAA2X;
        CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));

        s = base;
        s.crt = (base.crt == CrtFilterOption::On) ? CrtFilterOption::Off : CrtFilterOption::On;
        CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));

        s = base;
        s.rt = (base.rt == RayTracingOption::On) ? RayTracingOption::Off : RayTracingOption::On;
        // Ray tracing is the one difference between Enhanced and Enhanced Ray Tracing.
        const Preset rt_toggled = (p == Preset::Enhanced) ? Preset::EnhancedRayTracing :
            (p == Preset::EnhancedRayTracing) ? Preset::Enhanced : Preset::Custom;
        CHECK_EQ(val(matching_preset(s, interpolating())), val(rt_toggled));

        s = base;
        s.hdr = (base.hdr == HdrOption::On) ? HdrOption::Off : HdrOption::On;
        CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));
    }

    Settings s = preset(Preset::Authentic);
    s.crt_signal = CrtSignalOption::RGB;
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));

    s = preset(Preset::EnhancedRayTracing);
    s.rt_quality = RayTracingQuality::Quality;
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));

    s = preset(Preset::Max);
    s.hdr_picture = HdrPictureOption::Brighter;
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Custom));
}

TEST_CASE("hidden sub-options don't count: CRT signal with CRT off, RT quality with RT off, HDR picture with HDR off") {
    Settings s = preset(Preset::Enhanced);
    s.crt_signal = CrtSignalOption::RF;
    s.hdr_picture = HdrPictureOption::CrtDetail;
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Enhanced));

    s = preset(Preset::Authentic);
    s.rt_quality = RayTracingQuality::Ultra;
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Authentic));

    s = preset(Preset::Enhanced);
    s.rt_quality = RayTracingQuality::Performance;
    CHECK_EQ(val(matching_preset(s, interpolating())), val(Preset::Enhanced));
}

TEST_CASE("options the presets leave alone don't count") {
    // Window mode, resolution, HUD placement, CRT colors and beam simulator aren't part of Settings at all; a framerate
    // the game locks doesn't count either.
    Limits locked = interpolating();
    locked.smooth_rate = 0;
    Settings s = preset(Preset::Enhanced, locked);
    s.framerate = RefreshRate::Manual;
    CHECK_EQ(val(matching_preset(s, locked)), val(Preset::Enhanced));
}

TEST_CASE("MSAA above what the GPU offers counts as the GPU's highest level") {
    // A saved MSAA 4x on a 2x-only GPU runs as 2x, which is what Enhanced asks for there.
    Limits limits = interpolating();
    limits.max_msaa = Antialiasing::MSAA2X;
    Settings s = preset(Preset::Enhanced, interpolating());
    CHECK_EQ(val(s.msaa), val(Antialiasing::MSAA4X));
    CHECK_EQ(val(matching_preset(s, limits)), val(Preset::Enhanced));
}

TEST_CASE("Max on a 60 Hz display matches at the smooth rate, not at 120") {
    Limits limits = interpolating();
    limits.display_rate = 60;
    Settings s = preset(Preset::Max, limits);
    CHECK_EQ(val(matching_preset(s, limits)), val(Preset::Max));
    s.framerate = RefreshRate::Display;
    CHECK_EQ(val(matching_preset(s, limits)), val(Preset::Custom));
}

TEST_CASE("the preset labels are the short ones, so the row fits on one line") {
    CHECK(std::string(label(Preset::Authentic)) == "Authentic");
    CHECK(std::string(label(Preset::Enhanced)) == "Enhanced");
    CHECK(std::string(label(Preset::EnhancedRayTracing)) == "Enhanced RT");
    CHECK(std::string(label(Preset::Max)) == "Max");
    CHECK(std::string(label(Preset::Custom)) == "Custom");
    // "Enhanced Ray Tracing" wrapped onto two lines in the menu.
    size_t total = 0;
    for (Preset p : { Preset::Authentic, Preset::Enhanced, Preset::EnhancedRayTracing, Preset::Max, Preset::Custom }) {
        CHECK(std::string(label(p)).size() <= 11);
        total += std::string(label(p)).size();
    }
    CHECK(total <= 40);
}
