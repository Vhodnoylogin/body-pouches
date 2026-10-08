#pragma once
#include <cstdint>
namespace BodyPouches::Core
{
	class ButtonEdges
	{
	public:
		enum class Edge { None, Down, Up, Reset };
		void Clear() { _known = false; }
		Edge Sample(bool available, std::uint32_t context, bool down)
		{
			if (!available) { Clear(); return Edge::Reset; }
			if (!_known || _context != context) {
				_known = true; _context = context; _down = down;
				return Edge::Reset; // Also cancel any gesture belonging to the old device/binding.
			}
			if (_down == down) { return Edge::None; }
			_down = down;
			return down ? Edge::Down : Edge::Up;
		}
	private:
		bool _known{}, _down{};
		std::uint32_t _context{};
	};
}
