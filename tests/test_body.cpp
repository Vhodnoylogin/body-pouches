#include "Harness.h"
#include "game/Body.h"

using BodyPouches::Game::Body;

TEST(slot_geometry_uses_live_vrik_transform_not_engine_world)
{
    RE::NiAVObject root, bone;
    RE::PlayerCharacter player;
    player.thirdPerson = &root;
    RE::PlayerCharacter::instance = &player;
    root.children["NPC Spine1 [Spn1]"] = &bone;
    bone.world.translate = {100, 200, 300};
    bone.world.scale = 10;
    // A non-symmetric cyclic rotation: X->Y, Y->Z, Z->X.
    bone.previousWorld.rotate = {{{0,0,1}, {1,0,0}, {0,1,0}}};
    bone.previousWorld.translate = {10, 20, 30};
    bone.previousWorld.scale = 2;
    const auto native = Body::Bone(13);
    CHECK(native.has_value());
    if (native) {
        const auto world = Body::ToWorld(*native, {3,4,5});
        CHECK_EQ(world.x, 20);
        CHECK_EQ(world.y, 26);
        CHECK_EQ(world.z, 38);
        const auto local = Body::ToSlot(*native, {20,26,38});
        CHECK_EQ(local.x, 3);
        CHECK_EQ(local.y, 4);
        CHECK_EQ(local.z, 5);
        const auto z = Body::Axis(*native, 2);
        CHECK_EQ(z.x, 1);
        CHECK_EQ(z.y, 0);
        CHECK_EQ(z.z, 0);
    }
    // Reading the field again must see a live update, not the earlier transform.
    bone.previousWorld.translate = {40,50,60};
    const auto updated = Body::Bone(13);
    CHECK(updated.has_value());
    if (updated) CHECK_EQ(updated->translate.z, 60);
    RE::PlayerCharacter::instance = nullptr;
}

TEST(slot_geometry_refuses_missing_or_invalid_body)
{
    RE::PlayerCharacter::instance = nullptr;
    CHECK(!Body::Bone(13));
    RE::PlayerCharacter player;
    RE::PlayerCharacter::instance = &player;
    CHECK(!Body::Bone(13));
    RE::NiAVObject root;
    player.firstPerson = &root; // First person is not the VRIK slot skeleton.
    CHECK(!Body::Bone(13));
    player.thirdPerson = &root;
    CHECK(!Body::Bone(13));
    CHECK(!Body::Bone(0));
    CHECK(!Body::Bone(15));
    CHECK(!Body::BoneNamed(nullptr));
    RE::PlayerCharacter::instance = nullptr;
}
