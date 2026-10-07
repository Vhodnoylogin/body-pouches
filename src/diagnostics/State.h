#pragma once

#include "Config.h"
#include "core/PouchSet.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace BodyPouches::Diagnostics
{
	// Value-only data copied on the game thread; no world pointer leaves that thread.
	struct State
	{
		struct BodySlot
		{
			int slot{};
			std::optional<bool> suspended;
		};
		bool available{ false };
		std::string reason{ "not_initialized" };
		Settings settings;
		std::vector<Core::PouchState> pouches;
		std::vector<BodySlot> bodySlots;
		std::optional<bool> inputLeftHanded;
		std::uint64_t loadGeneration{};
		std::uint64_t callbackFrame{};
	};

	// A timed-out queued read must not later inspect a different world. Expiry is
	// also checked after copying; the listener never waits past its own deadline.
	class ReadGate
	{
	public:
		ReadGate(std::int64_t a_deadline, std::uint64_t a_epoch) :
			_deadline(a_deadline), _epoch(a_epoch) {}
		bool Begin(std::int64_t a_now, std::uint64_t a_epoch) noexcept
		{
			if (a_now >= _deadline || a_epoch != _epoch) {
				Cancel();
				return false;
			}
			int expected = 0;
			return _state.compare_exchange_strong(expected, 1);
		}
		void Cancel() noexcept { _state.store(2); }
		[[nodiscard]] bool Valid(std::int64_t a_now, std::uint64_t a_epoch) const noexcept
		{
			return _state.load() == 1 && a_now < _deadline && a_epoch == _epoch;
		}
		[[nodiscard]] std::int64_t Deadline() const noexcept { return _deadline; }

	private:
		std::atomic<int> _state{ 0 };
		std::int64_t _deadline;
		std::uint64_t _epoch;
	};
}
