#pragma once

#include <cstdint>

namespace BodyPouches::Core
{
	// Correlates one requested handover with its callback. The adapter supplies a
	// live engine handle (including its reuse bits), never a recyclable form ID.
	class PendingGrab
	{
	public:
		void Arm(std::uint32_t a_handle, std::int64_t a_deadline) noexcept
		{
			_handle = a_handle;
			_deadline = a_deadline;
		}
		void Clear() noexcept { _handle = 0; _deadline = 0; }
		[[nodiscard]] bool Active() const noexcept { return _handle != 0; }
		[[nodiscard]] std::uint32_t Handle() const noexcept { return _handle; }
		[[nodiscard]] std::int64_t Deadline() const noexcept { return _deadline; }
		[[nodiscard]] bool Expired(std::int64_t a_now) const noexcept
		{
			return Active() && a_now >= _deadline;
		}
		bool Confirm(std::uint32_t a_handle, std::int64_t a_now) noexcept
		{
			if (!Active() || a_handle != _handle || Expired(a_now)) {
				return false;
			}
			Clear();
			return true;
		}
	private:
		std::uint32_t _handle{ 0 };
		std::int64_t _deadline{ 0 };
	};
}
