#include "recompinput/controls_rules.h"

namespace recompinput::controls_rules {
    uint64_t controller_guid_hash(const std::string &serial, int vendor, int product, int version, int crc16) {
        // FNV-1a over the fields (the hash only lives in memory, so any stable function will do).
        uint64_t hash = 0xcbf29ce484222325ull;
        for (unsigned char c : serial) {
            hash ^= c;
            hash *= 0x100000001b3ull;
        }
        // Ends the serial, so "ab" + vendor 1 can't hash like "a" + bytes of another vendor.
        hash ^= 0xFFu;
        hash *= 0x100000001b3ull;
        auto mix = [&hash](int value) {
            const uint32_t bits = static_cast<uint32_t>(value);
            for (int i = 0; i < 4; i++) {
                hash ^= (bits >> (i * 8)) & 0xFFu;
                hash *= 0x100000001b3ull;
            }
        };
        mix(vendor);
        mix(product);
        mix(version);
        mix(crc16);
        return hash != 0 ? hash : 1;
    }

    std::string saved_controller_profile_key(const std::string &guid_key, const std::string &saved_profile_key,
        bool saved_profile_exists, bool saved_profile_custom, bool own_profile_exists) {
        if (saved_profile_exists && (saved_profile_custom || saved_profile_key == guid_key)) {
            return saved_profile_key;
        }
        return own_profile_exists ? guid_key : std::string();
    }

    bool controls_need_save(uint64_t revision, uint64_t saved_revision, bool binding_active) {
        return revision != saved_revision && !binding_active;
    }
}
