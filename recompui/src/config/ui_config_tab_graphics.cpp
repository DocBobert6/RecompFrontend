#include "librecomp/config.hpp"
#include "recompui/config.h"
#include "recompui/renderer.h"
#include "recompui/program_config.h"
#include "framerate_rules.h"
#include "graphics_presets.h"
#include "graphics_rules.h"
#include "util/steam_deck.h"
#include "render/rt64_crt_filter.h"
#include "render/rt64_hdr_output.h"
#include "render/rt64_wcw_raytracer.h"
#include "render/rt64_wcw_texture_edges.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <thread>

static bool created_graphics_config = false;

namespace recompui {
    namespace config {
        recomp::config::Config &get_graphics_config() {
            if (!created_graphics_config) {
                throw std::runtime_error("Graphics config has not been created yet. Call create_graphics_tab() first.");
            }
            return config::get_config(config::graphics::id);
        }

        static ultramodern::renderer::WindowMode wm_default() {
            return is_steam_deck() ? ultramodern::renderer::WindowMode::Fullscreen : ultramodern::renderer::WindowMode::Windowed;
        }

        using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
        static EnumOptionVector resolution_options = {
            {ultramodern::renderer::Resolution::Original, "Original", "Original"},
            {ultramodern::renderer::Resolution::Original2x, "Original2x", "Original 2x"},
            {ultramodern::renderer::Resolution::Auto, "Auto", "Auto"},
        };

        enum class DownsamplingOption {
            Off = 0,
            X2 = 2,
            X4 = 4,
        };
        static EnumOptionVector downsampling_options = {
            {DownsamplingOption::Off, "Off"},
            {DownsamplingOption::X2, "2x"},
            {DownsamplingOption::X4, "4x"},
        };

        // [wcw] Supersampling at Auto resolution: renders at twice the window's scale and box-filters it down
        // (graphics_rules.h). Stored like Downsampling Quality (Off = 0, 2x = 2).
        enum class SupersamplingOption {
            Off = 0,
            X2 = 2,
        };
        static EnumOptionVector supersampling_options = {
            {SupersamplingOption::Off, "Off"},
            {SupersamplingOption::X2, "2x"},
        };

        static EnumOptionVector window_mode_options = {
            {ultramodern::renderer::WindowMode::Windowed, "Windowed"},
            {ultramodern::renderer::WindowMode::Fullscreen, "Fullscreen"}
        };

        #if defined(_WIN32)
        #define ALLOW_D3D12
        #endif
        #if defined(_WIN32) || defined(__linux__)
        #define ALLOW_VULKAN
        #endif
        #if defined(__APPLE__)
        #define ALLOW_METAL
        #endif

        static EnumOptionVector graphics_api_options = {
            {ultramodern::renderer::GraphicsApi::Auto, "Auto"},
        #ifdef ALLOW_D3D12
            {ultramodern::renderer::GraphicsApi::D3D12, "D3D12"},
        #endif
        #ifdef ALLOW_VULKAN
            {ultramodern::renderer::GraphicsApi::Vulkan, "Vulkan"},
        #endif
        #ifdef ALLOW_METAL
            {ultramodern::renderer::GraphicsApi::Metal, "Metal"},
        #endif
        };

        // [wcw] 2D texture edges: point-sampled art (crowds, signs, HUD, fonts) as the N64 draws it, or with its texel
        // edges smoothed (sharp bilinear: texels stay crisp squares, seams blend over one screen pixel).
        enum class TextureEdgesOption {
            Original = 0,
            Smooth = 1
        };

        static EnumOptionVector texture_edges_options = {
            {TextureEdgesOption::Original, "Original", "Original"},
            {TextureEdgesOption::Smooth, "Smooth", "Smooth"},
        };

        // [wcw rt] Experimental hybrid ray tracing. The option enums (also used by the presets) are in graphics_presets.h.
        static EnumOptionVector ray_tracing_options = {
            {RayTracingOption::Off, "Off", "Off"},
            {RayTracingOption::On, "On", "On"},
        };

        static EnumOptionVector ray_tracing_quality_options = {
            {RayTracingQuality::Performance, "Performance", "Performance"},
            {RayTracingQuality::Balanced, "Balanced", "Balanced"},
            {RayTracingQuality::Quality, "Quality", "Quality"},
            {RayTracingQuality::Ultra, "Ultra", "Ultra"},
        };

        // [wcw crt] CRT filter: on/off, the video signal (a preset in crt/presets/ next to the
        // executable) and the TV's color standard.
        enum class CrtColorsOption {
            USA = 0,
            Japan = 1
        };

        static EnumOptionVector crt_filter_options = {
            {CrtFilterOption::Off, "Off", "Off"},
            {CrtFilterOption::On, "On", "On"},
        };

        static EnumOptionVector crt_signal_options = {
            {CrtSignalOption::Composite, "Composite", "Composite"},
            {CrtSignalOption::SVideo, "SVideo", "S-Video"},
            {CrtSignalOption::RF, "RF", "RF"},
            {CrtSignalOption::RGB, "RGB", "RGB"},
        };

        static EnumOptionVector crt_colors_options = {
            {CrtColorsOption::USA, "USA", "USA"},
            {CrtColorsOption::Japan, "Japan", "Japan"},
        };

        // [wcw hdr] HDR output.
        static EnumOptionVector hdr_options = {
            {HdrOption::Off, "Off", "Off"},
            {HdrOption::On, "On", "On"},
        };

        static EnumOptionVector hdr_picture_options = {
            {HdrPictureOption::Auto, "Auto", "Auto"},
            {HdrPictureOption::Brighter, "Brighter", "Brighter"},
            {HdrPictureOption::Balanced, "Balanced", "Balanced"},
            {HdrPictureOption::CrtDetail, "CrtDetail", "CRT Detail"},
            {HdrPictureOption::Custom, "Custom", "Custom"},
        };

        enum class CrtBeamOption {
            Off = 0,
            On = 1
        };

        static EnumOptionVector crt_beam_options = {
            {CrtBeamOption::Off, "Off", "Off"},
            {CrtBeamOption::On, "On", "On"},
        };

        static const char *crt_signal_preset(CrtSignalOption option) {
            switch (option) {
            case CrtSignalOption::SVideo: return "aki-svideo";
            case CrtSignalOption::RF: return "aki-rf";
            case CrtSignalOption::RGB: return "aki-rgb";
            case CrtSignalOption::Composite:
            default: return "aki-composite";
            }
        }

        static EnumOptionVector aspect_ratio_options = {
            {ultramodern::renderer::AspectRatio::Original, "Original"},
            {ultramodern::renderer::AspectRatio::Expand, "Expand"},
            // {ultramodern::renderer::AspectRatio::Manual, "Manual"},
        };

        static EnumOptionVector antialiasing_options = {
            {ultramodern::renderer::Antialiasing::None, "None"},
            {ultramodern::renderer::Antialiasing::MSAA2X, "MSAA2X", "2X"},
            {ultramodern::renderer::Antialiasing::MSAA4X, "MSAA4X", "4X"},
            {ultramodern::renderer::Antialiasing::MSAA8X, "MSAA8X", "8X"},
        };

        static EnumOptionVector refresh_rate_options = {
            {ultramodern::renderer::RefreshRate::Original, "Original"},
            {ultramodern::renderer::RefreshRate::Display, "Display"},
            {ultramodern::renderer::RefreshRate::Manual, "Manual"},
        };

        // [wcw hfr] With frame interpolation: Original and the game's smooth rate (stored as Manual at that rate).
        static EnumOptionVector smooth_rate_options(uint32_t smooth_rate) {
            return {
                {ultramodern::renderer::RefreshRate::Original, "Original", "30 (Original)"},
                {ultramodern::renderer::RefreshRate::Manual, "Manual", std::to_string(smooth_rate) + " (Smooth)"},
                // [wcw hfr] Experiment: twice the smooth rate. Stored under its own key and carried in the
                // Display slot (which these games never offer as such), applied as Manual at that rate.
                {ultramodern::renderer::RefreshRate::Display, "Smooth2x", std::to_string(smooth_rate * 2) + " (Experimental)"},
            };
        }

        static std::string smooth_rate_description(uint32_t smooth_rate) {
            return "<recomp-color warning>Experimental:</recomp-color> still being tuned, so expect rough edges. "
                "Sets the framerate. <recomp-color primary>30 (Original)</recomp-color> runs the game as it shipped. "
                "<recomp-color primary>" + std::to_string(smooth_rate) + " (Smooth)</recomp-color> generates in-between frames "
                "(frame interpolation): motion is smoother while the game itself, and your input, still update 30 times a second. "
                "<recomp-color primary>" + std::to_string(smooth_rate * 2) + " (Experimental)</recomp-color> generates three in-between frames; "
                "it needs a " + std::to_string(smooth_rate * 2) + " Hz (or faster) display."
                "<br /><br /><recomp-color primary>Note: rendering more frames costs more GPU time; with ray tracing on, "
                "each generated frame is ray traced too.</recomp-color>";
        }

        static EnumOptionVector hpfb_options = {
            {ultramodern::renderer::HighPrecisionFramebuffer::Auto, "Auto"},
            {ultramodern::renderer::HighPrecisionFramebuffer::On, "On"},
            {ultramodern::renderer::HighPrecisionFramebuffer::Off, "Off"},
        };

        static EnumOptionVector hud_ratio_mode_options = {
            {ultramodern::renderer::HUDRatioMode::Original, "Original"},
            {ultramodern::renderer::HUDRatioMode::Clamp16x9, "Clamp16x9", "16:9"},
            {ultramodern::renderer::HUDRatioMode::Full, "Expand"},
        };

        static const std::string get_downsampling_details(ultramodern::renderer::Resolution res_option, DownsamplingOption ds_option) {
            switch (res_option) {
                default:
                case ultramodern::renderer::Resolution::Auto:
                    return "";
                case ultramodern::renderer::Resolution::Original:
                    if (ds_option == DownsamplingOption::X2) {
                        return "Rendered in 480p and scaled to 240p";
                    } else if (ds_option == DownsamplingOption::X4) {
                        return "Rendered in 960p and scaled to 240p";
                    }
                    return "";
                case ultramodern::renderer::Resolution::Original2x:
                    if (ds_option == DownsamplingOption::X2) {
                        return "Rendered in 960p and scaled to 480p";
                    } else if (ds_option == DownsamplingOption::X4) {
                        return "Rendered in 4K and scaled to 480p";
                    }
                    return "";
            }
        }

        using OptionChangeContext = recomp::config::OptionChangeContext;

        static recomp::config::ConfigValueVariant get_graphics_value_variant(const std::string& key, OptionChangeContext change_context) {
            if (change_context == OptionChangeContext::Temporary) {
                return get_graphics_config().get_temp_option_value(key);
            } else {
                return get_graphics_config().get_option_value(key);
            }
        }

        template <typename T, typename ValType>
        T get_graphics_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return static_cast<T>(std::get<ValType>(get_graphics_value_variant(key, change_context)));
        }

        template <typename T = uint32_t>
        T get_graphics_enum_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return get_graphics_value<T, uint32_t>(key, change_context);
        }
    
        template <typename T = uint32_t>
        T get_graphics_number_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return get_graphics_value<T, double>(key, change_context);
        }

        static bool get_graphics_bool_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return std::get<bool>(get_graphics_value_variant(key, change_context));
        }

        static void determine_downsampling_display(ultramodern::renderer::Resolution res_option, OptionChangeContext change_context) {
            DownsamplingOption ds_opt = get_graphics_enum_value<DownsamplingOption>(graphics::options::ds_option, change_context);
            get_graphics_config().update_option_enum_details(graphics::options::ds_option, get_downsampling_details(res_option, ds_opt));
        }

        static std::string get_framerate_text(uint32_t refresh_rate) {
            // [wcw fix] Framerate is LOCKED to Original for this game — see the lock below.
            return
                "Sets the game's output framerate. This option does not affect gameplay."
                "<br />"
                "<br />"
                "<recomp-color warning>Locked to Original for this game:</recomp-color> frame interpolation warps geometry with this engine's display lists (it submits pre-multiplied matrices only, which defeats the renderer's motion matching)."
                "<br />"
                "<br />"
                "<recomp-color primary>Detected display refresh rate: " + std::to_string(refresh_rate) + "hz</recomp-color>";
        }

        // [wcw] WCW_SETTINGS_STRESS=<ms>: test harness for crashes on applying settings. Every <ms> it switches the
        // renderer between two sets of settings through the same calls as Apply (the owner's crash: fullscreen, 60 fps,
        // Expand, CRT on, RT Quality <-> windowed, 120 fps, 4:3, CRT off, RT Balanced). WCW_SETTINGS_STRESS_MASK picks
        // what changes (default 31): 1 window mode, 2 framerate, 4 aspect ratio, 8 CRT filter, 16 RT quality, 32 MSAA,
        // 64 resolution. Off by default; never saves the config.
        static void start_settings_stress() {
            const char *period_env = getenv("WCW_SETTINGS_STRESS");
            if (period_env == nullptr) {
                return;
            }

            const int period_ms = std::max(atoi(period_env), 100);
            const char *mask_env = getenv("WCW_SETTINGS_STRESS_MASK");
            const uint32_t mask = (mask_env != nullptr) ? uint32_t(atoi(mask_env)) : 31u;
            const ultramodern::renderer::GraphicsConfig base = ultramodern::renderer::get_graphics_config();
            const RT64::CrtFilter::Settings crt_base = RT64::CrtFilter::userSettings();
            const uint32_t smooth_rate = recompui::programconfig::get_frame_interpolation().smooth_rate;
            fprintf(stderr, "[wcw stress] switching settings every %d ms, mask %u\n", period_ms, mask);
            std::thread([=]() {
                for (uint64_t n = 1;; n++) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
                    const bool b = (n % 2) == 1;
                    ultramodern::renderer::GraphicsConfig config = base;
                    if (mask & 1) {
                        config.wm_option = b ? ultramodern::renderer::WindowMode::Windowed : ultramodern::renderer::WindowMode::Fullscreen;
                    }
                    if (mask & 2) {
                        config.rr_option = b ? ultramodern::renderer::RefreshRate::Display : ultramodern::renderer::RefreshRate::Manual;
                    }
                    if (mask & 4) {
                        config.ar_option = b ? ultramodern::renderer::AspectRatio::Original : ultramodern::renderer::AspectRatio::Expand;
                    }
                    if (mask & 32) {
                        config.msaa_option = b ? ultramodern::renderer::Antialiasing::None : ultramodern::renderer::Antialiasing::MSAA4X;
                    }
                    if (mask & 64) {
                        config.res_option = b ? ultramodern::renderer::Resolution::Original2x : ultramodern::renderer::Resolution::Auto;
                    }

                    recompui::framerate::apply_choice(config.rr_option, config.rr_manual_value, smooth_rate);
                    ultramodern::renderer::set_graphics_config(config);
                    if (mask & 16) {
                        RT64::WcwRaytracer::setUserSettings(true, b ? 1u : 2u);
                    }
                    if (mask & 8) {
                        RT64::CrtFilter::Settings crt = crt_base;
                        crt.enabled = !b;
                        RT64::CrtFilter::setUserSettings(crt);
                    }

                    fprintf(stderr, "[wcw stress] switch %llu: set %c\n", (unsigned long long)n, b ? 'B' : 'A');
                }
            }).detach();
        }

        // [wcw] Graphics presets (graphics_presets.h). The display's refresh rate and the GPU's MSAA levels arrive once
        // the renderer runs; until then the presets assume MSAA 4x (no preset depends on the display rate now, #165).
        static std::atomic<uint32_t> preset_display_rate = 0;
        static std::atomic<ultramodern::renderer::Antialiasing> preset_max_msaa = ultramodern::renderer::Antialiasing::MSAA4X;
        // Set while the tab itself changes the preset or its options, so those changes aren't taken as the player's.
        static bool syncing_preset = false;

        static EnumOptionVector preset_options = {
            {presets::Preset::Authentic, "Authentic", presets::label(presets::Preset::Authentic)},
            {presets::Preset::Enhanced, "Enhanced", presets::label(presets::Preset::Enhanced)},
            {presets::Preset::EnhancedRayTracing, "EnhancedRayTracing", presets::label(presets::Preset::EnhancedRayTracing)},
            {presets::Preset::Max, "Max", presets::label(presets::Preset::Max)},
            {presets::Preset::Custom, "Custom", presets::label(presets::Preset::Custom)},
        };

        // The options a preset sets.
        static const std::vector<std::string> &preset_option_ids() {
            static const std::vector<std::string> ids = {
                graphics::options::ar_option, graphics::options::rr_option, graphics::options::msaa_option,
                graphics::options::crt_filter_option, graphics::options::crt_signal_option,
                graphics::options::rt_option, graphics::options::rt_quality_option,
                graphics::options::hdr_option, graphics::options::hdr_picture,
            };
            return ids;
        }

        static presets::Limits preset_limits() {
            presets::Limits limits;
            limits.smooth_rate = recompui::programconfig::get_frame_interpolation().smooth_rate;
            limits.display_rate = preset_display_rate;
            limits.max_msaa = preset_max_msaa;
            return limits;
        }

        static presets::Settings preset_settings(OptionChangeContext change_context) {
            presets::Settings settings;
            settings.aspect_ratio = get_graphics_enum_value<ultramodern::renderer::AspectRatio>(graphics::options::ar_option, change_context);
            settings.framerate = get_graphics_enum_value<ultramodern::renderer::RefreshRate>(graphics::options::rr_option, change_context);
            settings.msaa = get_graphics_enum_value<ultramodern::renderer::Antialiasing>(graphics::options::msaa_option, change_context);
            settings.crt = get_graphics_enum_value<CrtFilterOption>(graphics::options::crt_filter_option, change_context);
            settings.crt_signal = get_graphics_enum_value<CrtSignalOption>(graphics::options::crt_signal_option, change_context);
            settings.rt = get_graphics_enum_value<RayTracingOption>(graphics::options::rt_option, change_context);
            settings.rt_quality = get_graphics_enum_value<RayTracingQuality>(graphics::options::rt_quality_option, change_context);
            settings.hdr = get_graphics_enum_value<HdrOption>(graphics::options::hdr_option, change_context);
            settings.hdr_picture = get_graphics_enum_value<HdrPictureOption>(graphics::options::hdr_picture, change_context);
            return settings;
        }

        // Picking a preset sets its options in the menu through set_option_value, like any change; Apply keeps them.
        // Sub-options of something the preset turns off (CRT signal, RT quality, HDR picture) keep the player's choice.
        static void stage_preset(presets::Preset preset) {
            const presets::Limits limits = preset_limits();
            const std::optional<presets::Settings> settings = presets::settings_for(preset, limits);
            if (!settings) {
                return;
            }

            recomp::config::Config &config = get_graphics_config();
            syncing_preset = true;
            config.update_option_value(graphics::options::ar_option, static_cast<uint32_t>(settings->aspect_ratio));
            if (limits.smooth_rate > 0) {
                config.update_option_value(graphics::options::rr_option, static_cast<uint32_t>(settings->framerate));
            }
            config.update_option_value(graphics::options::msaa_option, static_cast<uint32_t>(settings->msaa));
            config.update_option_value(graphics::options::crt_filter_option, static_cast<uint32_t>(settings->crt));
            if (settings->crt == CrtFilterOption::On) {
                config.update_option_value(graphics::options::crt_signal_option, static_cast<uint32_t>(settings->crt_signal));
            }
            config.update_option_value(graphics::options::rt_option, static_cast<uint32_t>(settings->rt));
            if (settings->rt == RayTracingOption::On) {
                config.update_option_value(graphics::options::rt_quality_option, static_cast<uint32_t>(settings->rt_quality));
            }
            config.update_option_value(graphics::options::hdr_option, static_cast<uint32_t>(settings->hdr));
            if (settings->hdr == HdrOption::On) {
                config.update_option_value(graphics::options::hdr_picture, static_cast<uint32_t>(settings->hdr_picture));
            }
            syncing_preset = false;
        }

        // After the player changes one of the preset's options: the preset shows what the options now match, which is
        // Custom unless they match a preset again. Following the options (rather than only ever switching to Custom)
        // also keeps the preset right when Cancel puts every option back.
        static void follow_preset(OptionChangeContext change_context) {
            if (syncing_preset || (change_context != OptionChangeContext::Temporary)) {
                return;
            }

            const presets::Preset matched = presets::matching_preset(preset_settings(OptionChangeContext::Temporary), preset_limits());
            if (matched != get_graphics_enum_value<presets::Preset>(graphics::options::preset, OptionChangeContext::Temporary)) {
                syncing_preset = true;
                get_graphics_config().update_option_value(graphics::options::preset, static_cast<uint32_t>(matched));
                syncing_preset = false;
            }
        }

        // On load the saved preset is set to what the saved options match (a config from before presets has none, and
        // the file can be edited by hand), through the same set/apply calls as the fullscreen toggle.
        static void reconcile_saved_preset() {
            const presets::Preset matched = presets::matching_preset(preset_settings(OptionChangeContext::Permanent), preset_limits());
            if (matched != get_graphics_enum_value<presets::Preset>(graphics::options::preset)) {
                syncing_preset = true;
                get_graphics_config().set_option_value(graphics::options::preset, static_cast<uint32_t>(matched));
                get_graphics_config().apply_option_value(graphics::options::preset);
                syncing_preset = false;
            }
        }

        static void apply_graphics_config() {
            ultramodern::renderer::GraphicsConfig new_config;
            new_config.developer_mode = get_graphics_bool_value(graphics::options::developer_mode);
            new_config.res_option = get_graphics_enum_value<ultramodern::renderer::Resolution>(graphics::options::res_option);
            new_config.wm_option = get_graphics_enum_value<ultramodern::renderer::WindowMode>(graphics::options::wm_option);
            new_config.hr_option = get_graphics_enum_value<ultramodern::renderer::HUDRatioMode>(graphics::options::hr_option);
            new_config.api_option = get_graphics_enum_value<ultramodern::renderer::GraphicsApi>(graphics::options::api_option);
            new_config.ar_option = get_graphics_enum_value<ultramodern::renderer::AspectRatio>(graphics::options::ar_option);
            new_config.msaa_option = get_graphics_enum_value<ultramodern::renderer::Antialiasing>(graphics::options::msaa_option);
            new_config.rr_option = get_graphics_enum_value<ultramodern::renderer::RefreshRate>(graphics::options::rr_option);
            new_config.hpfb_option = get_graphics_enum_value<ultramodern::renderer::HighPrecisionFramebuffer>(graphics::options::hpfb_option);
            new_config.rr_manual_value = get_graphics_number_value<int>(graphics::options::rr_manual_value);

            // [wcw hfr] Only Original, the game's fixed smooth rate or twice it (framerate_rules.h).
            recompui::framerate::apply_choice(new_config.rr_option, new_config.rr_manual_value,
                recompui::programconfig::get_frame_interpolation().smooth_rate);

            // [wcw] How many times the output size the game renders at: Downsampling Quality at Original and
            // Original 2x, Supersampling at Auto (graphics_rules.h).
            new_config.ds_option = recompui::graphics_rules::downsample_multiplier(new_config.res_option,
                get_graphics_enum_value<int>(graphics::options::ds_option), get_graphics_enum_value<int>(graphics::options::ss_option));

            ultramodern::renderer::set_graphics_config(new_config);

            // [wcw] Takes effect on the next frame.
            RT64::WcwTextureEdges::setSmooth(get_graphics_enum_value<TextureEdgesOption>(graphics::options::texture_edges_option) == TextureEdgesOption::Smooth);

            // [wcw rt] Takes effect on the next frame (no renderer restart needed).
            RT64::WcwRaytracer::setUserSettings(
                get_graphics_enum_value<RayTracingOption>(graphics::options::rt_option) == RayTracingOption::On,
                uint32_t(get_graphics_enum_value<RayTracingQuality>(graphics::options::rt_quality_option)));

            // [wcw crt] Takes effect on the next frame; the first use of a preset compiles its shaders.
            RT64::CrtFilter::Settings crt_settings;
            crt_settings.enabled = get_graphics_enum_value<CrtFilterOption>(graphics::options::crt_filter_option) == CrtFilterOption::On;
            crt_settings.preset = crt_signal_preset(get_graphics_enum_value<CrtSignalOption>(graphics::options::crt_signal_option));
            // guest-advanced "LUT Colors": 0 = none, 1 = NTSC-J.
            const bool crt_japan = get_graphics_enum_value<CrtColorsOption>(graphics::options::crt_colors_option) == CrtColorsOption::Japan;
            crt_settings.parameters.emplace_back("TNTC", crt_japan ? 1.0f : 0.0f);
            crt_settings.beamSimulator = get_graphics_enum_value<CrtBeamOption>(graphics::options::crt_beam_option) == CrtBeamOption::On;
            RT64::CrtFilter::setUserSettings(crt_settings);

            graphics::apply_hdr_settings();

            static std::once_flag stress_once;
            std::call_once(stress_once, start_settings_stress);
        }

        void graphics::update_msaa_supported(bool supported) {
            recomp::config::Config &config = get_graphics_config();
            if (!supported) {
                preset_max_msaa = ultramodern::renderer::Antialiasing::None;
                config.update_option_enum_details(
                    graphics::options::msaa_option,
                    supported ? "Available" : "Not available (missing sample positions support)"
                );
                config.update_option_disabled(
                    graphics::options::msaa_option,
                    true
                );
            } else {
                // [wcw] RT64's levels are listed in the same order as ultramodern's (None, 2X, 4X, 8X).
                const auto max_msaa = static_cast<ultramodern::renderer::Antialiasing>(recompui::renderer::RT64MaxMSAA());
                // The presets use up to 4X.
                preset_max_msaa = std::min(max_msaa, ultramodern::renderer::Antialiasing::MSAA4X);
                for (auto level : { ultramodern::renderer::Antialiasing::MSAA2X, ultramodern::renderer::Antialiasing::MSAA4X, ultramodern::renderer::Antialiasing::MSAA8X }) {
                    if (!recompui::graphics_rules::msaa_level_supported(level, max_msaa)) {
                        config.update_enum_option_disabled(graphics::options::msaa_option, static_cast<uint32_t>(level), true);
                    }
                }
            }
        }

        static const std::string hdr_description =
            "<recomp-color warning>Experimental:</recomp-color> still being tuned, so expect rough edges. "
            "Outputs in HDR on displays with Windows HDR turned on. Recommended with the CRT filter: it uses the display's extra "
            "brightness for a stronger, more authentic phosphor mask without dimming the picture."
            "<br />"
            "<br />"
            "<recomp-color primary>Takes effect after restarting the game.</recomp-color>";

        static const std::string hdr_picture_description =
            "How bright the game's white is with HDR on. The CRT filter spends the display's brightness above it on the phosphor mask. "
            "<recomp-color primary>Auto</recomp-color> picks for your display: as bright as your desktop, or with the CRT filter, "
            "dim enough for a full-strength mask while staying comfortable. "
            "<recomp-color primary>Brighter</recomp-color> matches your desktop. "
            "<recomp-color primary>CRT Detail</recomp-color> gives the CRT filter its strongest mask. "
            "<recomp-color primary>Balanced</recomp-color> sits between the two.";

        static std::string format_nits(float nits) {
            return std::to_string(int(std::lround(nits))) + " nits";
        }

        // [wcw hdr] The HDR Picture readout: the display's figures, the resulting game white and CRT mask, and advice
        // (RT64::WcwLogic::hdrAdvice). Follows the menu's values as they change, before Apply.
        static void update_hdr_picture_readout(OptionChangeContext change_context = OptionChangeContext::Permanent) {
            const RT64::HdrOutput::Status status = RT64::HdrOutput::status();
            std::string readout = "<br /><br />";
            if (!status.active) {
                readout += "Turn on HDR and restart the game to see what this gives on your display.";
            }
            else {
                const bool crt_on = get_graphics_enum_value<CrtFilterOption>(graphics::options::crt_filter_option, change_context) == CrtFilterOption::On;
                const bool beam_setting = get_graphics_enum_value<CrtBeamOption>(graphics::options::crt_beam_option, change_context) == CrtBeamOption::On;
                RT64::WcwLogic::HdrCrt crt;
                crt.on = crt_on;
                crt.baseMaskStrength = status.crt.on ? status.crt.baseMaskStrength : RT64::WcwLogic::HdrCrt().baseMaskStrength;
                // The beam simulator only runs on a fast enough display; once the filter runs it reports whether it does.
                crt.beam = crt_on && beam_setting && (status.crt.on ? status.crt.beam : true);
                const auto picture = RT64::WcwLogic::HdrPicture(get_graphics_enum_value<HdrPictureOption>(graphics::options::hdr_picture, change_context));
                const float custom_nits = float(get_graphics_number_value<int>(graphics::options::hdr_custom_nits, change_context));
                const float paper_white = RT64::WcwLogic::hdrPaperWhite(picture, custom_nits, status.display, crt);

                readout += "Your display: ";
                if (status.display.peakNits > 0.0f) {
                    readout += format_nits(status.display.peakNits) + " peak";
                    if (status.display.fullFrameNits > 0.0f) {
                        readout += ", " + format_nits(status.display.fullFrameNits) + " full-screen";
                    }
                }
                else {
                    readout += "peak brightness not reported (assuming " + format_nits(RT64::WcwLogic::HdrUnknownPeakNits) + ")";
                }

                readout += ".<br />Game white: <recomp-color primary>" + format_nits(paper_white) + "</recomp-color>";
                if (crt_on) {
                    const RT64::WcwLogic::HdrMask mask = RT64::WcwLogic::hdrMask(paper_white, status.display.peakNits, crt.baseMaskStrength, crt.beam);
                    readout += ". CRT phosphor mask: <recomp-color primary>" + std::to_string(int(std::lround(mask.strength * 100.0f))) + "%</recomp-color>";
                    if (mask.strength >= 0.995f) {
                        readout += " (full strength)";
                    }
                }

                readout += ".<br /><br />";
                switch (RT64::WcwLogic::hdrAdvice(paper_white, status.display, crt)) {
                case RT64::WcwLogic::HdrAdvice::TooBright:
                    readout += "<recomp-color warning>Brighter than your display can hold across the whole screen, so bright scenes will dim.</recomp-color> Try Auto.";
                    break;
                case RT64::WcwLogic::HdrAdvice::LowHeadroom:
                    readout += "<recomp-color warning>Little brightness left over for the CRT filter's mask.</recomp-color> Try Auto or CRT Detail.";
                    break;
                case RT64::WcwLogic::HdrAdvice::Dim:
                    readout += "<recomp-color warning>This is quite dim.</recomp-color> Try Auto or Brighter.";
                    break;
                case RT64::WcwLogic::HdrAdvice::Good:
                default:
                    readout += "<recomp-color success>Good for your display.</recomp-color>";
                    break;
                }
            }

            get_graphics_config().update_option_description(graphics::options::hdr_picture, hdr_picture_description + readout);
        }

        // [wcw hdr] On/off takes effect when the renderer next creates its swap chain (a restart);
        // the picture choice applies immediately.
        void graphics::apply_hdr_settings() {
            if (!created_graphics_config) {
                return;
            }

            RT64::HdrOutput::Settings hdr_settings;
            hdr_settings.enabled = get_graphics_enum_value<HdrOption>(graphics::options::hdr_option) == HdrOption::On;
            hdr_settings.picture = RT64::WcwLogic::HdrPicture(get_graphics_enum_value<HdrPictureOption>(graphics::options::hdr_picture));
            hdr_settings.customNits = float(get_graphics_number_value<int>(graphics::options::hdr_custom_nits));
            RT64::HdrOutput::setUserSettings(hdr_settings);
            update_hdr_picture_readout();
        }

        void graphics::update_hdr_status(bool requested, bool active) {
            std::string status;
            if (active) {
                status = "<br /><br /><recomp-color success>HDR is active.</recomp-color>";
            }
            else if (requested) {
                status = "<br /><br /><recomp-color warning>Windows HDR is off for this display, so the game is using SDR.</recomp-color> "
                    "Turn on \"Use HDR\" in Windows display settings, then restart the game.";
            }

            get_graphics_config().update_option_description(graphics::options::hdr_option, hdr_description + status);
            update_hdr_picture_readout();
        }

        void graphics::update_refresh_rate(uint32_t refresh_rate) {
            recomp::config::Config& config = get_graphics_config();
            preset_display_rate = refresh_rate;
            std::string framerate_text = get_framerate_text(refresh_rate);
            // [wcw hfr] The smooth-rate option describes itself; only the locked option shows the display rate.
            if (recompui::programconfig::get_frame_interpolation().smooth_rate == 0) {
                config.update_option_description(graphics::options::rr_option, framerate_text);
                config.update_option_description(graphics::options::rr_manual_value, framerate_text);
            }
        }

        void graphics::toggle_fullscreen() {
            auto current_wm = get_graphics_enum_value<ultramodern::renderer::WindowMode>(graphics::options::wm_option, OptionChangeContext::Temporary);
            auto new_wm = current_wm == ultramodern::renderer::WindowMode::Windowed ? ultramodern::renderer::WindowMode::Fullscreen : ultramodern::renderer::WindowMode::Windowed;
            get_graphics_config().set_option_value(graphics::options::wm_option, static_cast<uint32_t>(new_wm));
            get_graphics_config().apply_option_value(graphics::options::wm_option);
            apply_graphics_config();
        }

        recomp::config::Config &create_graphics_tab(const std::string &name) {
            created_graphics_config = true;

            recomp::config::Config &config = recompui::config::create_config_tab(name, graphics::id, true);
            config.set_save_callback(apply_graphics_config);
            config.set_load_callback([]() {
                reconcile_saved_preset();
                apply_graphics_config();
            });

            config.add_bool_option(
                graphics::options::developer_mode,
                "Dev Mode",
                "Enables developer features.",
                false,
                true
            );

            // [wcw] Graphics presets. The options' defaults are Enhanced's, so a new player starts on Enhanced.
            const uint32_t smooth_rate = recompui::programconfig::get_frame_interpolation().smooth_rate;
            const presets::Settings enhanced = presets::settings_for(presets::Preset::Enhanced, preset_limits()).value();
            config.add_enum_option(
                graphics::options::preset,
                "Preset",
                "Sets the main graphics options in one go; you can still change any of them afterwards."
                "<br />"
                "<br />"
                "<recomp-color primary>Authentic</recomp-color>: 4:3, 30 fps, CRT filter."
                "<br />"
                "<recomp-color primary>Enhanced</recomp-color> (default): widescreen, smooth motion, MSAA 4x."
                "<br />"
                "<recomp-color primary>Enhanced RT</recomp-color>: Enhanced plus ray tracing (Balanced)."
                "<br />"
                "<recomp-color primary>Max</recomp-color>: Ultra ray tracing, HDR, smooth motion."
                "<br />"
                "<recomp-color primary>Custom</recomp-color>: your own mix (shown when no preset matches)."
                "<br />"
                "<br />"
                "Ray tracing needs an RTX-class GPU with D3D12; HDR needs Windows HDR and a game restart.",
                preset_options,
                presets::Preset::Enhanced
            );
            {
                // Custom is only ever shown, never picked: a locked value (radio_rules.h), drawn as selected when current.
                config.update_enum_option_disabled(graphics::options::preset, static_cast<uint32_t>(presets::Preset::Custom), true);
                config.add_option_change_callback(
                    graphics::options::preset,
                    [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant, OptionChangeContext change_context) {
                        if (!syncing_preset && (change_context == OptionChangeContext::Temporary)) {
                            stage_preset(static_cast<presets::Preset>(std::get<uint32_t>(cur_value)));
                        }
                    }
                );
            }

            config.add_enum_option(
                graphics::options::res_option,
                "Resolution",
                "Sets the output resolution of the game. <recomp-color primary>Original</recomp-color> matches the game's original 240p resolution. <recomp-color primary>Original 2x</recomp-color> will render at 480p. <recomp-color primary>Auto</recomp-color> will scale based on the game window's resolution.",
                resolution_options,
                ultramodern::renderer::Resolution::Auto
            );
            {
                config.add_option_change_callback(
                    graphics::options::res_option,
                    [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant prev_value, OptionChangeContext change_context) {
                        auto new_opt = static_cast<ultramodern::renderer::Resolution>(std::get<uint32_t>(cur_value));
                        determine_downsampling_display(new_opt, change_context);
                    }
                );
            }

            config.add_enum_option(
                graphics::options::ds_option,
                "Downsampling Quality",
                "Renders at a higher resolution and scales it down to the output resolution for increased quality. For <recomp-color primary>Original</recomp-color> and <recomp-color primary>Original 2x</recomp-color> resolution; <recomp-color primary>Auto</recomp-color> has <recomp-color primary>Supersampling</recomp-color> instead."
                "<br />"
                "<br />"
                "Note: <recomp-color primary>4x</recomp-color> downsampling quality at <recomp-color primary>Original 2x</recomp-color> resolution may cause performance issues on low end devices, as it will cause the game to render <recomp-color warning>at almost 4k internal resolution</recomp-color>.",
                downsampling_options,
                DownsamplingOption::Off
            );
            {
                config.add_option_change_callback(
                    graphics::options::ds_option,
                    [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant prev_value, OptionChangeContext change_context) {
                        auto resolution_opt = get_graphics_enum_value<ultramodern::renderer::Resolution>(graphics::options::res_option, change_context);
                        determine_downsampling_display(resolution_opt, change_context);
                    }
                );

                // [wcw] Auto shows Supersampling in its place (it used to show this option disabled).
                config.add_option_hidden_dependency(
                    graphics::options::ds_option,
                    graphics::options::res_option,
                    ultramodern::renderer::Resolution::Auto
                );
    
                config.on_json_parse_option(graphics::options::ds_option, [](const nlohmann::json& j) {
                    return j.get<uint32_t>();
                });
    
                config.on_json_serialize_option(graphics::options::ds_option, [](const recomp::config::ConfigValueVariant& value) {
                    return nlohmann::json(std::get<uint32_t>(value));
                });
            }

            // [wcw] Supersampling: Downsampling Quality's counterpart at Auto resolution.
            config.add_enum_option(
                graphics::options::ss_option,
                "Supersampling",
                "Renders at twice the resolution and scales it down to the window for smoother edges. Unlike MSAA, it also "
                "smooths cut-out edges such as ropes, cages and ladders, and steadies shimmering textures."
                "<br />"
                "<br />"
                "<recomp-color warning>Note:</recomp-color> <recomp-color primary>2x</recomp-color> renders four times the pixels "
                "and is demanding at high window sizes: about 4K internally for a 1080p window. Ray tracing still traces at the window's size. "
                "At 120 fps every in-between frame is rendered at that size too: with a 4K window, expect well under 120 fps and "
                "several GB of video memory.",
                supersampling_options,
                SupersamplingOption::Off
            );
            {
                config.add_option_hidden_dependency(
                    graphics::options::ss_option,
                    graphics::options::res_option,
                    ultramodern::renderer::Resolution::Original,
                    ultramodern::renderer::Resolution::Original2x
                );

                config.on_json_parse_option(graphics::options::ss_option, [](const nlohmann::json& j) {
                    // Configs saved before this option existed have no value for it (null): use the default.
                    if (!j.is_number_unsigned()) {
                        return uint32_t(SupersamplingOption::Off);
                    }
                    return j.get<uint32_t>();
                });

                config.on_json_serialize_option(graphics::options::ss_option, [](const recomp::config::ConfigValueVariant& value) {
                    return nlohmann::json(std::get<uint32_t>(value));
                });
            }

            config.add_enum_option(
                graphics::options::ar_option,
                "Aspect Ratio",
                "Sets the horizontal aspect ratio. <recomp-color primary>Original</recomp-color> uses the game's original 4:3 aspect ratio. <recomp-color primary>Expand</recomp-color> will adjust to match the game window's aspect ratio.",
                aspect_ratio_options,
                enhanced.aspect_ratio
            );

            config.add_enum_option(
                graphics::options::wm_option,
                "Window Mode",
                "Sets whether the game should display <recomp-color primary>Windowed</recomp-color> or <recomp-color primary>Fullscreen</recomp-color>. You can also use <recomp-color primary>F11</recomp-color> or <recomp-color primary>Alt + Enter</recomp-color> to toggle this option.",
                window_mode_options,
                wm_default()
            );

            // [wcw hfr] Framerate: games that declare frame interpolation (programconfig::set_frame_interpolation)
            // offer "30 (Original)" / "<rate> (Smooth)", defaulting to Original. Games that don't stay locked to
            // Original: these engines submit only combined matrices, which RT64 can't interpolate on its own.
            static EnumOptionVector framerate_options = (smooth_rate > 0) ? smooth_rate_options(smooth_rate) : refresh_rate_options;
            config.add_enum_option(
                graphics::options::rr_option,
                (smooth_rate > 0) ? "Framerate (Experimental)" : "Framerate",
                (smooth_rate > 0) ? smooth_rate_description(smooth_rate) : get_framerate_text(60),
                framerate_options,
                enhanced.framerate
            );

            config.add_number_option(
                graphics::options::rr_manual_value,
                "",
                get_framerate_text(60),
                20.0, 240.0, 1.0, 0, false, 60.0
            );
            {
                if (smooth_rate > 0) {
                    // The rate box never applies: the smooth rate is fixed per game. (A hidden dependency
                    // would show it again whenever the option is Manual.)
                    config.update_option_hidden(graphics::options::rr_manual_value, true);
                }
                else {
                    config.add_option_hidden_dependency(
                        graphics::options::rr_manual_value,
                        graphics::options::rr_option,
                        ultramodern::renderer::RefreshRate::Original,
                        ultramodern::renderer::RefreshRate::Display
                    );
                    config.update_option_disabled(graphics::options::rr_option, true);
                }

                // Only a saved Smooth choice (stored as Manual) survives, and only when the game has a smooth
                // rate. Everything else loads as Original, including Display: older builds coerced the option
                // to Original at load without rewriting the file, so a stale Display must not turn on
                // interpolation by itself.
                config.on_json_parse_option(graphics::options::rr_option, [smooth_rate](const nlohmann::json &j) -> recomp::config::ConfigValueVariant {
                    return static_cast<uint32_t>(recompui::framerate::parse_saved_option(j, smooth_rate));
                });
            }

            config.add_enum_option(
                graphics::options::msaa_option,
                "MS Anti-Aliasing",
                "Sets the multisample anti-aliasing (MSAA) quality level. This reduces jagged edges in the final image at the expense of rendering performance."
                "<br />"
                "<br />"
                "<recomp-color primary>Note: This option won't be available if your GPU does not support programmable MSAA sample positions, as it is currently required to avoid rendering glitches.</recomp-color>",
                antialiasing_options,
                enhanced.msaa
            );

            config.add_enum_option(
                graphics::options::hr_option,
                "HUD Placement",
                "Adjusts the placement of HUD elements to fit the selected aspect ratio. <recomp-color primary>Expand</recomp-color> will use the aspect ratio of the game's output window.",
                hud_ratio_mode_options,
                ultramodern::renderer::HUDRatioMode::Clamp16x9
            );

            config.add_enum_option(
                graphics::options::api_option,
                "Graphics API",
                "Selects the graphics API to use.",
                graphics_api_options,
                ultramodern::renderer::GraphicsApi::Auto,
                true
            );

            config.add_enum_option(
                graphics::options::hpfb_option,
                "High Precision Framebuffer",
                "Sets whether to use a high precision framebuffer.",
                hpfb_options,
                ultramodern::renderer::HighPrecisionFramebuffer::Off,
                true
            );

            config.add_enum_option(
                graphics::options::crt_filter_option,
                "CRT Filter (Experimental)",
                "<recomp-color warning>Experimental:</recomp-color> still being tuned, so expect rough edges. "
                "Simulates a period CRT television: the video signal, scanlines, phosphor mask and glow. "
                "Looks best at higher window resolutions."
                "<br />"
                "<br />"
                "The first use of each signal takes a few seconds to prepare.",
                crt_filter_options,
                enhanced.crt
            );

            config.add_enum_option(
                graphics::options::crt_signal_option,
                "CRT Signal",
                "<recomp-color primary>Composite</recomp-color>: how most players saw the N64. "
                "<recomp-color primary>S-Video</recomp-color>: a cleaner picture on a Trinitron. "
                "<recomp-color primary>RF</recomp-color>: the RF switch, the softest and noisiest. "
                "<recomp-color primary>RGB</recomp-color>: a sharp professional monitor (enthusiast look; a stock N64 has no RGB output).",
                crt_signal_options,
                CrtSignalOption::Composite
            );

            config.add_enum_option(
                graphics::options::crt_colors_option,
                "TV Colors",
                "The color standard of the simulated TV. <recomp-color primary>Japan</recomp-color> uses NTSC-J colors (cooler whites), as on a Japanese set.",
                crt_colors_options,
                CrtColorsOption::USA
            );
            config.add_enum_option(
                graphics::options::crt_beam_option,
                "CRT Beam Simulator (Experimental)",
                "<recomp-color warning>Experimental:</recomp-color> still being tuned, so expect rough edges. "
                "Simulates the CRT's rolling scan for CRT-like motion clarity (Blur Busters CRT Beam Simulator)."
                "<br />"
                "<br />"
                "<recomp-color primary>Requires the CRT Filter, V-Sync and a 120 Hz or faster display.</recomp-color> "
                "At higher framerates the simulated CRT refreshes at the framerate when the display is at least twice as fast "
                "(e.g. 120 fps on a 240 Hz+ display). "
                "Works best at 240 Hz and up, and on OLED. Not recommended with variable refresh rate (G-Sync/FreeSync) "
                "or local dimming. It has no effect otherwise.",
                crt_beam_options,
                CrtBeamOption::Off
            );
            {
                config.add_option_hidden_dependency(
                    graphics::options::crt_beam_option,
                    graphics::options::crt_filter_option,
                    CrtFilterOption::Off
                );
                config.add_option_hidden_dependency(
                    graphics::options::crt_signal_option,
                    graphics::options::crt_filter_option,
                    CrtFilterOption::Off
                );
                config.add_option_hidden_dependency(
                    graphics::options::crt_colors_option,
                    graphics::options::crt_filter_option,
                    CrtFilterOption::Off
                );
            }

            config.add_enum_option(
                graphics::options::hdr_option,
                "HDR (Experimental)",
                hdr_description,
                hdr_options,
                enhanced.hdr
            );

            config.add_enum_option(
                graphics::options::hdr_picture,
                "HDR Picture",
                hdr_picture_description,
                hdr_picture_options,
                HdrPictureOption::Auto
            );

            config.add_number_option(
                graphics::options::hdr_custom_nits,
                "HDR Custom Brightness",
                "Brightness of the game's white, in nits. Lower leaves more of the display's brightness for the CRT filter's mask.",
                80, 600, 10, 0, false, 200
            );
            {
                config.add_option_hidden_dependency(
                    graphics::options::hdr_picture,
                    graphics::options::hdr_option,
                    HdrOption::Off
                );
                config.add_option_hidden_dependency(
                    graphics::options::hdr_custom_nits,
                    graphics::options::hdr_picture,
                    HdrPictureOption::Auto, HdrPictureOption::Brighter, HdrPictureOption::Balanced, HdrPictureOption::CrtDetail
                );
            }

            config.add_enum_option(
                graphics::options::texture_edges_option,
                "2D Texture Edges",
                "How pixel-art textures (crowds, signs, the HUD, fonts) look at high resolutions. "
                "<recomp-color primary>Original</recomp-color> keeps the N64's sharp texel blocks. "
                "<recomp-color primary>Smooth</recomp-color> keeps every texel a crisp square but softens the edges between them "
                "over one screen pixel, so they don't look jagged or crawl when they move.",
                texture_edges_options,
                TextureEdgesOption::Original
            );

            config.add_enum_option(
                graphics::options::rt_option,
                "Ray Tracing (Experimental)",
                "<recomp-color warning>Experimental:</recomp-color> still being tuned, so expect rough edges. "
                "Adds ray-traced shadows from the arena lights and contact shadows, replacing the game's blob shadows."
                "<br />"
                "<br />"
                "<recomp-color primary>Requires an RTX-class GPU with DirectX Raytracing 1.1 and the D3D12 graphics API.</recomp-color> It has no effect otherwise.",
                ray_tracing_options,
                enhanced.rt
            );

            config.add_enum_option(
                graphics::options::rt_quality_option,
                "Ray Tracing Quality",
                "<recomp-color primary>Performance</recomp-color> traces at reduced resolution with the fewest rays and skips ambient occlusion. "
                "<recomp-color primary>Balanced</recomp-color> traces at reduced resolution and adds soft ambient occlusion where surfaces meet "
                "(under arms, between the legs, where wrestlers touch the mat). "
                "<recomp-color primary>Quality</recomp-color> traces every pixel up to 1440p (at 4K, two thirds of the lines) for sharper shadows and deeper ambient occlusion. "
                "<recomp-color primary>Ultra</recomp-color> keeps Quality's look with twice the rays and traces every pixel at any resolution, for the cleanest result; "
                "it costs the most and suits fast GPUs.",
                ray_tracing_quality_options,
                enhanced.rt_quality
            );
            {
                config.add_option_hidden_dependency(
                    graphics::options::rt_quality_option,
                    graphics::options::rt_option,
                    RayTracingOption::Off
                );
            }

            // An option has one change callback, so this registers them for both uses: the HDR Picture readout follows
            // the choices that change it while the menu is open, and the preset follows the options it sets.
            {
                const std::vector<std::string> readout_ids = { graphics::options::hdr_picture, graphics::options::hdr_custom_nits,
                    graphics::options::crt_filter_option, graphics::options::crt_beam_option };
                const std::vector<std::string> &preset_ids = preset_option_ids();
                std::vector<std::string> ids = readout_ids;
                for (const std::string &option : preset_ids) {
                    if (std::find(ids.begin(), ids.end(), option) == ids.end()) {
                        ids.push_back(option);
                    }
                }

                for (const std::string &option : ids) {
                    const bool readout = std::find(readout_ids.begin(), readout_ids.end(), option) != readout_ids.end();
                    const bool preset = std::find(preset_ids.begin(), preset_ids.end(), option) != preset_ids.end();
                    config.add_option_change_callback(option,
                        [readout, preset](recomp::config::ConfigValueVariant, recomp::config::ConfigValueVariant, OptionChangeContext change_context) {
                            if (readout) {
                                update_hdr_picture_readout(change_context);
                            }
                            if (preset) {
                                follow_preset(change_context);
                            }
                        }
                    );
                }
            }

            return config;
        }
    }
} // namespace recompui
