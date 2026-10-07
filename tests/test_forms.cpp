#include "Harness.h"
#include "game/Forms.h"

TEST(runtime_potion_without_source_file_round_trips_by_full_id)
{
    RE::TESDataHandler data;
    RE::TESDataHandler::instance = &data;
    RE::AlchemyItem brewed;
    brewed.id = 0xFF001234;
    RE::TESForm::runtime[brewed.id] = &brewed;
    try {
        const auto item = BodyPouches::Game::Describe(&brewed);
        CHECK(item.key.plugin.empty());
        CHECK_EQ(item.key.localId, brewed.id);
        CHECK_EQ(BodyPouches::Game::Lookup(item.key), &brewed);
    } catch (const std::exception& e) {
        Harness::Fail(__FILE__, __LINE__, e.what());
    }
    RE::TESForm::runtime.clear();
    RE::TESDataHandler::instance = nullptr;
}

TEST(runtime_clone_with_inherited_source_file_keeps_runtime_identity)
{
    RE::TESFile source{"Skyrim.esm", 0, 0};
    RE::TESForm clone{0xFF001235, &source};
    const auto key = BodyPouches::Game::KeyOf(&clone);
    CHECK(key.plugin.empty());
    CHECK_EQ(key.localId, clone.id);
}

TEST(plugin_and_light_plugin_forms_keep_local_identity)
{
    RE::TESFile regular{"PotionMod.esp", 0x12, 0};
    RE::TESFile light{"LightPotion.esp", 0xFE, 0xABC};
    RE::TESForm a{0x12001234, &regular};
    RE::TESForm b{0xFEABC801, &light};
    const auto ka = BodyPouches::Game::KeyOf(&a);
    const auto kb = BodyPouches::Game::KeyOf(&b);
    CHECK_EQ(ka.plugin, "PotionMod.esp");
    CHECK_EQ(ka.localId, 0x1234u);
    CHECK_EQ(kb.plugin, "LightPotion.esp");
    CHECK_EQ(kb.localId, 0x801u);
    CHECK(!BodyPouches::Game::KeyOf(nullptr).IsSet());
}
