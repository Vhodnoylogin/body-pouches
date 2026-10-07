#pragma once

namespace BodyPouches::Diagnostics
{
	void Connect();
	void Invalidate() noexcept;
	void SetLoadResult(bool a_succeeded) noexcept;
}
