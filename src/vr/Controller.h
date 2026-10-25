#pragma once
#include <cstdint>
namespace BodyPouches::VR
{
	struct ControllerButton { bool available{}; bool down{}; std::uint32_t context{}; };
	// Read-only original runtime interface. VRIK filters Skyrim's later button events.
	ControllerButton ReadControllerButton(bool left, bool trigger);
}
