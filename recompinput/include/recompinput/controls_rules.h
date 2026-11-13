#pragma once

#include <cstdint>
#include <string>

#include "json/json.hpp"

// [wcw fix] Rules for saving and loading the controls (controls.json), kept free of SDL and UI state so they can be
// unit tested (recompui/tests/controls_rules_tests.cpp, aki-roadmap#303; the code is in
// recompinput/src/controls_rules.cpp). profiles.cpp and the UI call these; a change to the rules belongs here, with a test.
namespace recompinput::controls_rules {
    // The lookup hash of a controller GUID. It is never written to controls.json, so it must be rebuilt from the saved
    // fields on load: before #303 a loaded GUID kept hash 0, every saved controller collapsed onto one entry and the
    // file's "controllers" list came back with pads linked to each other's profiles. Includes the serial, like the profile
    // key, so two pads of one model with different serials (DualShock 4, Switch Pro) don't share an entry. Never 0.
    uint64_t controller_guid_hash(const std::string &serial, int vendor, int product, int version, int crc16);

    // Which profile a controller saved in controls.json links to on load. Every per-pad profile is keyed by its pad's
    // GUID string, so a link to a different pad's non-custom profile is the pre-#303 corruption; it falls back to the
    // pad's own profile when that exists. Custom profiles (shared between pads) keep their link. Returns the profile key
    // to use, or "" to skip the entry (the pad's profile is made when it is plugged in).
    std::string saved_controller_profile_key(const std::string &guid_key, const std::string &saved_profile_key,
        bool saved_profile_exists, bool saved_profile_custom, bool own_profile_exists);

    // Whether the controls must be written now. The mappings change on every bind, clear, reset and new pad; they are
    // saved as soon as they change (not only when the Controls tab closes: quitting from the menu or closing the window
    // with the tab open lost every change, #303), but not while a bind is waiting for its input.
    bool controls_need_save(uint64_t revision, uint64_t saved_revision, bool binding_active);
}

namespace recompinput {
    struct ControllerGUID {
        uint64_t hash{};
        std::string serial;
        int vendor{};
        int product{};
        int version{};
        int crc16{};
    };

    inline void to_json(nlohmann::json& j, const ControllerGUID& guid) {
        j = nlohmann::json{ {"serial", guid.serial}, {"vendor", guid.vendor}, {"product", guid.product}, {"version", guid.version}, {"crc16", guid.crc16} };
    }

    inline void from_json(const nlohmann::json& j, ControllerGUID& guid) {
        j.at("serial").get_to(guid.serial);
        j.at("vendor").get_to(guid.vendor);
        j.at("product").get_to(guid.product);
        j.at("version").get_to(guid.version);
        j.at("crc16").get_to(guid.crc16);
        // [wcw fix] The hash isn't saved; rebuild it (a loaded GUID used to keep hash 0, aki-roadmap#303).
        guid.hash = controls_rules::controller_guid_hash(guid.serial, guid.vendor, guid.product, guid.version, guid.crc16);
    }
}
