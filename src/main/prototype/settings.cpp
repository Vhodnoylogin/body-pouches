// SPDX-License-Identifier: MIT
#include "settings.h"
#include "Paths.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace BodyPouches::Prototype {
namespace {
using Json = nlohmann::json;
Json Read(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 65536) throw std::runtime_error("configuration exceeds 64 KiB");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("configuration cannot be opened");
    Json result;
    input >> result;
    if (!result.is_object()) throw std::runtime_error("configuration must be an object");
    return result;
}
PouchSettings Decode(const Json& json) {
    PouchSettings result;
    for (const char* key : {"position", "rotationDegrees"})
        if (json.contains(key) && (!json.at(key).is_array() || json.at(key).size() != 3))
            throw std::runtime_error("position and rotation must each have three numbers");
    result.enabled = json.value("enabled", result.enabled);
    result.entryHaptics = json.value("entryHaptics", result.entryHaptics);
    result.importLegacySlot = json.value("importLegacySlot", result.importLegacySlot);
    result.logLevel = json.value("logLevel", result.logLevel);
    result.anchor = json.value("anchor", result.anchor);
    result.position = json.value("position", result.position);
    result.radius = json.value("radius", result.radius);
    const auto degrees = json.value("rotationDegrees", std::array<float, 3>{});
    if (result.importLegacySlot < 0 || result.importLegacySlot > 14 || result.anchor.empty() ||
        result.anchor.size() >= 96 || result.anchor.find('\0') != std::string::npos ||
        !std::isfinite(result.radius) || result.radius < 1 || result.radius > 50)
        throw std::runtime_error("invalid legacy slot, anchor or radius");
    for (unsigned char ch : result.anchor)
        if (ch < 32 || ch > 126) throw std::runtime_error("anchor must use printable ASCII");
    for (float p : result.position)
        if (!std::isfinite(p) || std::abs(p) > 500) throw std::runtime_error("invalid local position");
    for (float angle : degrees)
        if (!std::isfinite(angle) || std::abs(angle) > 360) throw std::runtime_error("invalid rotation");
    if (result.logLevel != "trace" && result.logLevel != "debug" && result.logLevel != "info" &&
        result.logLevel != "warn" && result.logLevel != "error" && result.logLevel != "off")
        throw std::runtime_error("invalid log level");
    constexpr float radians = 0.017453292519943295f;
    const float x = degrees[0] * radians, y = degrees[1] * radians, z = degrees[2] * radians;
    const float cx = std::cos(x), sx = std::sin(x), cy = std::cos(y), sy = std::sin(y), cz = std::cos(z), sz = std::sin(z);
    // Row-major Rz * Ry * Rx, passed as an authored default to Framework.
    result.rotation = {cz*cy, cz*sy*sx-sz*cx, cz*sy*cx+sz*sx,
        sz*cy, sz*sy*sx+cz*cx, sz*sy*cx-cz*sx, -sy, cy*sx, cy*cx};
    return result;
}
Json Encode(const PouchSettings& settings) {
    return {{"enabled", settings.enabled}, {"entryHaptics", settings.entryHaptics},
        {"importLegacySlot", settings.importLegacySlot}, {"logLevel", settings.logLevel},
        {"anchor", settings.anchor}, {"position", settings.position},
        {"rotationDegrees", std::array<float, 3>{}}, {"radius", settings.radius}};
}
}
PouchSettings LoadPouchSettings() {
    const auto path = Paths::Root() / "bodypouches2.json";
    try {
        if (std::filesystem::exists(path)) return Decode(Read(path));
        PouchSettings defaults;
        // Respect a deliberate disable in the old mod on first migration only.
        // Native slot numbers/modes/poses are not interpreted as Framework geometry.
        if (std::filesystem::exists(Paths::ConfigFile())) {
            try {
                const auto old = Read(Paths::ConfigFile());
                if (old.contains("pouches") && old.at("pouches").is_array() && old.at("pouches").empty())
                    defaults.enabled = false;
                defaults.logLevel = Decode(Json{{"logLevel", old.value("logLevel", defaults.logLevel)}}).logLevel;
            } catch (const std::exception& error) {
                spdlog::warn("legacy settings not imported: {}", error.what());
            }
        }
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary);
        if (!output) throw std::runtime_error("configuration cannot be created");
        output << Encode(defaults).dump(2) << '\n';
        if (!output) throw std::runtime_error("configuration write failed");
        return defaults;
    } catch (const std::exception& error) {
        spdlog::error("Body Pouches 2 configuration unavailable ({}); bag disabled, file retained", error.what());
        PouchSettings disabled;
        disabled.enabled = false;
        return disabled;
    }
}
}
