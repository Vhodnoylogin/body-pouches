#pragma once
#include <cstdint>

namespace BodyPouches::Core
{
	// A release event may be resolved after VRIK only in its own HIGGS update and
	// world. This is an event marker, never a remembered slot/reach/hand position.
	class CurrentUpdateRelease
	{
	public:
		void Arm(std::uint64_t a_world, std::uint64_t a_update) noexcept
		{
			_world = a_world;
			_update = a_update;
			_armed = true;
		}
		void Clear() noexcept { _armed = false; }
		[[nodiscard]] bool Active() const noexcept { return _armed; }
		bool Take(std::uint64_t a_world, std::uint64_t a_update) noexcept
		{
			const bool valid = _armed && a_world == _world && a_update == _update;
			Clear();
			return valid;
		}
	private:
		bool _armed{};
		std::uint64_t _world{}, _update{};
	};
}
