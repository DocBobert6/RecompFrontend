#include "model_pack_menu.h"

#include <algorithm>

#include "json/json.hpp"

namespace recompui::config::model_pack_menu {
    bool valid_pack_id(std::string_view id) {
        if (id.empty() || id.size() > 31 || id == "off") {   // "off" starts the mod's off lines
            return false;
        }
        for (char c : id) {
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
                return false;
            }
        }
        return true;
    }

    // models.txt as lines (without their line ends), a UTF-8 BOM dropped. "\r\n", "\n" and "\r" all end a line, as
    // in the mod's reader.
    static std::vector<std::string> split_lines(const std::string &text) {
        std::vector<std::string> lines;
        size_t start = (text.compare(0, 3, "\xEF\xBB\xBF") == 0) ? 3 : 0;
        std::string cur;
        for (size_t i = start; i < text.size(); i++) {
            const char c = text[i];
            if (c == '\n' || c == '\r') {
                lines.push_back(cur);
                cur.clear();
                if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
                    i++;
                }
            }
            else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) {
            lines.push_back(cur);
        }
        return lines;
    }

    // A line's words up to a '#' comment, split on spaces and tabs (the mod's reader).
    static std::vector<std::string> words(const std::string &line) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : line) {
            if (c == '#') {
                break;
            }
            if (c == ' ' || c == '\t') {
                if (!cur.empty()) {
                    out.push_back(cur);
                    cur.clear();
                }
            }
            else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) {
            out.push_back(cur);
        }
        return out;
    }

    // A number as the mod's parse_int reads it: decimal, or hex after 0x; -1 when it isn't one (or is out of range).
    static long parse_number(const std::string &s) {
        size_t i = 0;
        bool neg = false;
        if (i < s.size() && s[i] == '-') {
            neg = true;
            i++;
        }
        int base = 10;
        if (i + 1 < s.size() && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
            base = 16;
            i += 2;
        }
        if (i >= s.size()) {
            return -1;
        }
        long v = 0;
        for (; i < s.size(); i++) {
            const char c = s[i];
            int d = (c >= '0' && c <= '9') ? c - '0' : (base == 16 && c >= 'a' && c <= 'f') ? c - 'a' + 10
                  : (base == 16 && c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
            if (d < 0 || d >= base || v > 0xFFFFFF) {
                return -1;
            }
            v = v * base + d;
        }
        return neg ? -v : v;
    }

    // "<pack> slot N" with N 1-4 (as the mod reads N): N, else 0.
    static int slot_of_line(const std::vector<std::string> &w, std::string *pack = nullptr) {
        if (w.size() != 3 || w[1] != "slot" || !valid_pack_id(w[0])) {
            return 0;
        }
        const long n = parse_number(w[2]);
        if (n < 1 || n > slot_count) {
            return 0;
        }
        if (pack != nullptr) {
            *pack = w[0];
        }
        return int(n);
    }

    static bool is_off_line(const std::vector<std::string> &w, std::string_view pack_id) {
        return w.size() == 2 && w[0] == "off" && w[1] == pack_id;
    }

    static bool pack_is_off(const std::vector<std::string> &lines, std::string_view pack_id) {
        for (const std::string &line : lines) {
            if (is_off_line(words(line), pack_id)) {
                return true;
            }
        }
        return false;
    }

    // The mod's pack_for_slot for slot lines: the first line for the slot decides; if its pack is off, the slot is
    // empty (the mod doesn't look further).
    static std::string slot_pack_of(const std::vector<std::string> &lines, int slot) {
        for (const std::string &line : lines) {
            std::string pack;
            if (slot_of_line(words(line), &pack) == slot) {
                return pack_is_off(lines, pack) ? std::string() : pack;
            }
        }
        return "";
    }

    std::string slot_pack(const std::string &text, int slot) {
        return slot_pack_of(split_lines(text), slot);
    }

    int pack_slot(const std::string &text, std::string_view pack_id) {
        const std::vector<std::string> lines = split_lines(text);
        for (int slot = 1; slot <= slot_count; slot++) {
            if (slot_pack_of(lines, slot) == pack_id) {
                return slot;
            }
        }
        return 0;
    }

    std::string assign(const std::string &text, std::string_view pack_id, int slot) {
        const std::string line_for_pack = std::string(pack_id) + " slot " + std::to_string(slot) + "\n";
        std::string out;
        bool placed = (slot == 0);
        for (const std::string &line : split_lines(text)) {
            const std::vector<std::string> w = words(line);
            std::string pack;
            const int line_slot = slot_of_line(w, &pack);
            const bool slot_line = line_slot != 0 && (pack == pack_id || (slot != 0 && line_slot == slot));
            if (slot_line && !placed) {
                out += line_for_pack;   // where the line it replaces was
                placed = true;
            }
            if (!slot_line && !(slot != 0 && is_off_line(w, pack_id))) {
                out += line;
                out += "\n";
            }
        }
        if (!placed) {
            out += line_for_pack;
        }
        return out;
    }

    Pack parse_pack_json(std::string_view id, const std::string &pack_json) {
        Pack pack;
        pack.id = std::string(id);
        pack.name = pack.id;
        const nlohmann::json j = nlohmann::json::parse(pack_json, nullptr, false);
        if (!j.is_object()) {
            return pack;
        }
        auto text = [&j](const char *key) -> std::string {
            auto it = j.find(key);
            return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
        };
        if (!text("name").empty()) {
            pack.name = text("name");
        }
        pack.author = text("author");
        pack.game = text("game");
        return pack;
    }

    std::vector<Row> rows(const std::vector<Pack> &packs, const std::string &text) {
        const std::vector<std::string> lines = split_lines(text);
        std::vector<Row> out;
        for (const Pack &pack : packs) {
            Row row;
            row.pack = pack;
            row.slot = pack_slot(text, pack.id);
            if (pack_is_off(lines, pack.id)) {
                row.note = "Off in models.txt";
            }
            else {
                for (const std::string &line : lines) {
                    const std::vector<std::string> w = words(line);
                    if (w.size() >= 3 && w[0] == pack.id && w[1] == "wrestler") {
                        row.note = "Also replaces wrestlers by id (models.txt)";
                        break;
                    }
                }
            }
            out.push_back(row);
        }
        std::stable_sort(out.begin(), out.end(), [](const Row &a, const Row &b) {
            return (a.pack.name != b.pack.name) ? (a.pack.name < b.pack.name) : (a.pack.id < b.pack.id);
        });
        return out;
    }
}
