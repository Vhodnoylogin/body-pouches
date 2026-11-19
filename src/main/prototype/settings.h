// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <string>

namespace BodyPouches::Prototype {
struct PouchSettings {
    bool enabled = true;
    bool entryHaptics = true;
    int importLegacySlot = 13;
    std::string logLevel = "info";
    std::string anchor = "NPC Pelvis [Pelv]";
    std::array<float, 3> position{18, 0, -4};
    std::array<float, 9> rotation{1, 0, 0, 0, 1, 0, 0, 0, 1};
    float radius = 10;
};
// Startup filesystem boundary; never writes the released mod's configuration.
[[nodiscard]] PouchSettings LoadPouchSettings();
}
