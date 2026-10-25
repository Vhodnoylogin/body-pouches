#include "Harness.h"
#include "vr/VrikLink.h"
#include "vr/Handshake.h"
#include "core/SlotPlan.h"
#include <map>
#include <string>
#include <vector>
#include <Windows.h>

namespace {
class NativeVrik final : public vrikPluginApi::IVrikInterface001 {
public:
    unsigned int build = 80700;
    std::map<std::string, double> settings;
    std::vector<std::pair<std::string, double>> writes;
    bool suspended{};
    unsigned int getBuildNumber() override { return build; }
    RE::NiPoint3 getCameraOffsettingAmount() override { return {}; }
    void AddHolsterAttemptCallback(HolsterAttemptCallback) override {}
    bool SetSlotSuspended(int, bool value) override { suspended = value; return true; }
    bool GetSlotSuspended(int) override { return suspended; }
    bool VrikGetSlotDisplayed(int) override { return false; }
    void VrikSetSlotForArt(int, ::BGSArtObject*) override {}
    double getSettingDouble(const char* name) override { return settings[name]; }
    void setSettingDouble(const char* name, double value) override {
        writes.emplace_back(name, value); settings[name] = value;
    }
    int GetHolsterSlotInReach(bool) override { return 0; }
    bool CanBeHolstered(bool) override { return false; }
};
struct Fixture {
    NativeVrik api;
    BodyPouches::VR::VrikLink link;
    Fixture() {
        BodyPouches::VR::fixtureVrikApi = &api;
        CHECK(link.Connect());
    }
    ~Fixture() { BodyPouches::VR::fixtureVrikApi = nullptr; }
};
}

TEST(exclusive_visible_empty_slot_never_enables_a_weapon_category)
{
    Fixture f;
    f.api.settings["visibleSlot13"] = 1;
    CHECK(f.link.IsDetectable(13, true));
    CHECK(!f.link.IsDetectable(13));
    CHECK(!f.link.SwitchOn(13, true));
    CHECK(f.api.writes.empty());
    const auto view = f.link.SeeSlot(13, true);
    const auto plan = BodyPouches::Core::PlanFor({13, view.detectable, true, false});
    CHECK(plan.suspend);
    CHECK(!plan.switchOn);
    CHECK(f.link.SetSuspended(13, true));
    CHECK(f.api.suspended);
    CHECK(f.api.writes.empty());
}

TEST(exclusive_hidden_slot_opt_in_changes_only_visibility)
{
    Fixture f;
    CHECK(!f.link.IsDetectable(13, true));
    CHECK(f.link.SwitchOn(13, true));
    CHECK_EQ(f.api.writes.size(), 1u);
    CHECK_EQ(f.api.writes[0].first, "visibleSlot13");
    CHECK_EQ(f.api.writes[0].second, 1.0);
    CHECK(f.link.IsDetectable(13, true));
    CHECK(!f.link.SwitchOn(13, true));
    CHECK_EQ(f.api.writes.size(), 1u);
}

TEST(disabled_shared_slot_keeps_native_enable_policy)
{
    Fixture f;
    CHECK(f.link.SwitchOn(13));
    CHECK_EQ(f.api.writes.size(), 2u);
    CHECK_EQ(f.api.settings["allowSmallSlot13"], 1.0);
    CHECK_EQ(f.api.settings["visibleSlot13"], 1.0);
    CHECK(f.link.IsDetectable(13));
    CHECK(!f.api.suspended);
}

TEST(shared_existing_weapon_filter_is_preserved_when_visibility_is_enabled)
{
    Fixture f;
    f.api.settings["allowTorchSlot13"] = 1.0;
    CHECK(!f.link.IsDetectable(13));
    CHECK(f.link.SwitchOn(13));
    CHECK_EQ(f.api.writes.size(), 1u);
    CHECK_EQ(f.api.writes[0].first, "visibleSlot13");
    CHECK(f.link.IsDetectable(13));
    CHECK_EQ(f.api.settings["allowSmallSlot13"], 0.0);
}

TEST(slot_enable_refuses_invalid_slot_or_missing_framework)
{
    Fixture f;
    CHECK(!f.link.SwitchOn(0, true));
    CHECK(!f.link.SwitchOn(15));
    CHECK(!f.link.IsDetectable(15, true));
    BodyPouches::VR::VrikLink absent;
    CHECK(!absent.SwitchOn(13, true));
    CHECK(f.api.writes.empty());
}

namespace {
const vrik_native_suspension::Api* VNS_CALL RefusedQuery(std::uint32_t, std::uint32_t) { return nullptr; }
const vrik_native_suspension::Api* VNS_CALL ThrowingQuery(std::uint32_t, std::uint32_t) { throw 1; }
}
TEST(advertised_unavailable_suspension_service_never_falls_back_to_unowned_write)
{
    for (const auto query : {RefusedQuery, ThrowingQuery}) {
        fixtureSuspensionQuery = reinterpret_cast<FARPROC>(query);
        {
            Fixture f;
            CHECK(!f.link.SetSuspended(13, true));
            f.link.MaintainSuspensions();
            CHECK(!f.api.suspended);
            CHECK(f.api.writes.empty());
        }
        fixtureSuspensionQuery = nullptr;
    }
}
