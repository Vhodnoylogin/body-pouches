#include "Harness.h"
#include "Config.h"
#include "Paths.h"

#include <chrono>
#include <fstream>

namespace
{
    struct IsolatedConfig
    {
        std::filesystem::path previous{std::filesystem::current_path()};
        std::filesystem::path root{std::filesystem::temp_directory_path() /
            ("bodypouches-config-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
        IsolatedConfig()
        {
            std::filesystem::create_directories(root);
            std::filesystem::current_path(root);
            std::filesystem::create_directories(BodyPouches::Paths::ConfigFile().parent_path());
        }
        ~IsolatedConfig()
        {
            std::filesystem::current_path(previous);
            std::filesystem::remove_all(root);
        }
        void Put(const char* json) { std::ofstream(BodyPouches::Paths::ConfigFile()) << json; }
    };
}

TEST(missing_pouches_preserves_default_and_other_user_settings)
{
    IsolatedConfig files;
    files.Put(R"({"language":"russian","logLevel":"debug","mayEnableSlots":true})");
    const auto settings = BodyPouches::LoadSettings();
    CHECK_EQ(settings.language, "russian");
    CHECK(settings.mayEnableSlots);
    CHECK_EQ(settings.pouches.size(), 1u);
    if (!settings.pouches.empty()) CHECK_EQ(settings.pouches.front().slot, 13);
    const auto again = BodyPouches::LoadSettings();
    CHECK_EQ(again.pouches.size(), 1u);
}

TEST(explicit_empty_pouches_still_disables_them)
{
    IsolatedConfig files;
    files.Put(R"({"pouches":[]})");
    CHECK(BodyPouches::LoadSettings().pouches.empty());
    CHECK(BodyPouches::LoadSettings().pouches.empty());
}

TEST(custom_pouch_survives_missing_optional_fields_and_reload)
{
    IsolatedConfig files;
    files.Put(R"({"pouches":[{"slot":2,"mode":"shared"}]})");
    for (int i = 0; i < 2; ++i) {
        const auto settings = BodyPouches::LoadSettings();
        CHECK_EQ(settings.pouches.size(), 1u);
        if (!settings.pouches.empty()) CHECK_EQ(settings.pouches.front().slot, 2);
    }
}
