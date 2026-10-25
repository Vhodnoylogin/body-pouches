#pragma once

#include <cmath>
#include <cstdint>

namespace BodyPouches::Core
{
	// Shared slots stay owned by VRIK. This gesture covers only the case where
	// VRIK's weapon filter offers no candidate to an empty weapon hand.
	class SharedWithdrawal
	{
	public:
		void Clear() noexcept { _slot = 0; }
		[[nodiscard]] int Slot() const noexcept { return _slot; }
		void Press(int a_slot, double a_distance, double a_radius, std::int64_t a_now) noexcept
		{
			Clear();
			if (a_slot > 0 && Valid(a_distance, a_radius) && a_distance <= a_radius) {
				_slot = a_slot;
				_pressedAt = a_now;
			}
		}
		// A cancelled gesture cannot arm again by entering the zone with a held
		// button. A fresh physical press inside the zone is always required.
		[[nodiscard]] int Update(bool a_allowed, double a_distance, double a_radius,
			std::int64_t a_now) noexcept
		{
			if (!_slot) { return 0; }
			if (!a_allowed || !Valid(a_distance, a_radius) || a_now < _pressedAt ||
				a_now - _pressedAt >= 3000) {
				Clear();
				return 0;
			}
			if (a_distance <= a_radius) { return 0; }
			const int slot = _slot;
			Clear();
			return slot;
		}
	private:
		static bool Valid(double a_distance, double a_radius) noexcept
		{
			return std::isfinite(a_distance) && a_distance >= 0 &&
				std::isfinite(a_radius) && a_radius > 0;
		}
		int _slot{};
		std::int64_t _pressedAt{};
	};
}
