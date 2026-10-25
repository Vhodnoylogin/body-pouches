#include "Controller.h"
#include <Windows.h>
#pragma warning(push, 0)
#include "openvr.h"
#pragma warning(pop)
namespace BodyPouches::VR
{
	ControllerButton ReadControllerButton(bool left, bool trigger)
	{
		// Use Skyrim's existing runtime; never initialize/shut down OpenVR or change input.
		const auto module = GetModuleHandleW(L"openvr_api.dll");
		if (!module) { return {}; }
		using GetInterface = void* (*)(const char*, vr::EVRInitError*);
		const auto get = reinterpret_cast<GetInterface>(GetProcAddress(module, "VR_GetGenericInterface"));
		if (!get) { return {}; }
		vr::EVRInitError error = vr::VRInitError_None;
		auto* system = static_cast<vr::IVRSystem*>(get(vr::IVRSystem_Version, &error));
		if (!system || error != vr::VRInitError_None) { return {}; }
		const auto device = system->GetTrackedDeviceIndexForControllerRole(
			left ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);
		if (device == vr::k_unTrackedDeviceIndexInvalid || !system->IsTrackedDeviceConnected(device)) { return {}; }
		vr::VRControllerState_t state{};
		if (!system->GetControllerState(device, &state, sizeof(state))) { return {}; }
		const auto button = trigger ? vr::k_EButton_SteamVR_Trigger : vr::k_EButton_Grip;
		return { true, (state.ulButtonPressed & vr::ButtonMaskFromId(button)) != 0,
			(device << 8) | static_cast<std::uint32_t>(button) };
	}
}
