#pragma once

namespace BodyPouches::Core
{
	// A failed load blocks its stale world, but must not block the next new game.
	class LoadGate
	{
	public:
		void Begin() noexcept { _state = State::Loading; }
		void Complete() noexcept { _state = State::Idle; }
		void Fail() noexcept { _state = State::Failed; }
		void Revert() noexcept { if (_state != State::Loading) { _state = State::Idle; } }
		[[nodiscard]] bool Blocked() const noexcept { return _state != State::Idle; }
	private:
		enum class State { Idle, Loading, Failed };
		State _state{ State::Idle };
	};
}
