#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace BodyPouches::Diagnostics
{
	// Bounded value-only causal history. Capture does not allocate or wait for a
	// reader, query the world, write a log or call back into another mod.
	enum class Event
	{
		LoadBegin, LoadEnd, DrawQueued, DrawTask, DrawWorldCancelled,
		DrawGuardRejected, DrawPolicyRejected, RemoveRequested, RemoveResult,
		PendingArmed, GrabCallback, GrabAccepted, GrabRejected, PendingExpired,
		StowQueued, StowTask, StowWorldCancelled, StowSerialCancelled,
		RegrabCancelledStow, PickupRequested, PickupReturned
	};
	inline const char* Name(Event a_event) noexcept
	{
		switch (a_event) {
		case Event::LoadBegin: return "load.begin";
		case Event::LoadEnd: return "load.end";
		case Event::DrawQueued: return "draw.queued";
		case Event::DrawTask: return "draw.task";
		case Event::DrawWorldCancelled: return "draw.world_cancelled";
		case Event::DrawGuardRejected: return "draw.guard_rejected";
		case Event::DrawPolicyRejected: return "draw.policy_rejected";
		case Event::RemoveRequested: return "draw.remove_requested";
		case Event::RemoveResult: return "draw.remove_result";
		case Event::PendingArmed: return "pending.armed";
		case Event::GrabCallback: return "grab.callback";
		case Event::GrabAccepted: return "grab.accepted";
		case Event::GrabRejected: return "grab.rejected";
		case Event::PendingExpired: return "pending.expired";
		case Event::StowQueued: return "stow.queued";
		case Event::StowTask: return "stow.task";
		case Event::StowWorldCancelled: return "stow.world_cancelled";
		case Event::StowSerialCancelled: return "stow.serial_cancelled";
		case Event::RegrabCancelledStow: return "stow.regrab_cancelled";
		case Event::PickupRequested: return "stow.pickup_requested";
		case Event::PickupReturned: return "stow.pickup_returned";
		}
		return "unknown";
	}
	struct TraceEvent
	{
		Event event{};
		std::uint64_t sequence{}, worldGeneration{}, higgsUpdate{}, callbackFrame{};
		std::int64_t monotonicMs{}, deadlineMs{};
		int physicalHand{ -1 }; // -1=not hand-specific, 0=left, 1=right
		int secondaryHand{ -1 };
		int slot{}, quantity{};
		std::uint32_t handle{}, pendingHandle{};
		std::uint64_t causalId{}, dropSerial{}, enqueuedWorldGeneration{};
		bool inHiggsUpdate{}, pendingActive{};
	};
	struct TraceSnapshot
	{
		bool enabled{}, available{};
		std::uint64_t nextSequence{ 1 }, overwritten{}, busyDrops{};
		std::vector<TraceEvent> events;
	};
	class Trace
	{
	public:
		static constexpr std::size_t kCapacity = 256;
		// Set once at data loading, before gameplay callbacks. Loads retain history.
		void Enable(bool a_enabled) noexcept { _enabled.store(a_enabled); }
		[[nodiscard]] bool Enabled() const noexcept { return _enabled.load(); }
		void Record(TraceEvent a_event) noexcept
		{
			if (!Enabled()) { return; }
			a_event.sequence = _next.fetch_add(1);
			std::unique_lock guard(_mutex, std::try_to_lock);
			if (!guard.owns_lock()) { ++_busyDrops; return; }
			if (_size == kCapacity) { ++_overwritten; }
			else { ++_size; }
			_events[_write] = a_event;
			_write = (_write + 1) % kCapacity;
		}
		[[nodiscard]] TraceSnapshot Snapshot()
		{
			TraceSnapshot result;
			result.enabled = Enabled();
			result.nextSequence = _next.load();
			result.busyDrops = _busyDrops.load();
			if (!result.enabled) { return result; }
			std::unique_lock guard(_mutex, std::try_to_lock);
			if (!guard.owns_lock()) { return result; }
			result.available = true;
			result.overwritten = _overwritten;
			result.events.reserve(_size);
			const auto first = (_write + kCapacity - _size) % kCapacity;
			for (std::size_t i = 0; i < _size; ++i) { result.events.push_back(_events[(first + i) % kCapacity]); }
			return result;
		}
	private:
		std::atomic<bool> _enabled{ false };
		std::atomic<std::uint64_t> _next{ 1 }, _busyDrops{};
		std::mutex _mutex;
		std::array<TraceEvent, kCapacity> _events{};
		std::size_t _write{}, _size{};
		std::uint64_t _overwritten{};
	};
}
