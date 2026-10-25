#pragma once

namespace BodyPouches::Diagnostics
{
#ifdef BODYPOUCHES_WITH_DEVBENCH
	void Connect();
	void Invalidate() noexcept;
	void SetLoadResult(bool a_succeeded) noexcept;
#else
    // Keep lifecycle call sites identical; release builds contain no bridge.
    inline void Connect() noexcept {}
    inline void Invalidate() noexcept {}
    inline void SetLoadResult(bool) noexcept {}
#endif
}
