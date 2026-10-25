#pragma once
#include <RE/Skyrim.h>
// Adapter boundary double; this intentionally does not qualify the real001 ABI.
namespace vrikPluginApi {
class IVrikInterface001 {
public:
    using HolsterAttemptCallback = bool (*)(int, bool, bool);
    virtual ~IVrikInterface001() = default;
    virtual unsigned int getBuildNumber() = 0;
    virtual RE::NiPoint3 getCameraOffsettingAmount() = 0;
    virtual void AddHolsterAttemptCallback(HolsterAttemptCallback) = 0;
    virtual bool SetSlotSuspended(int, bool) = 0;
    virtual bool GetSlotSuspended(int) = 0;
    virtual bool VrikGetSlotDisplayed(int) = 0;
    virtual void VrikSetSlotForArt(int, ::BGSArtObject*) = 0;
    virtual double getSettingDouble(const char*) = 0;
    virtual void setSettingDouble(const char*, double) = 0;
    virtual int GetHolsterSlotInReach(bool) = 0;
    virtual bool CanBeHolstered(bool) = 0;
};
}
