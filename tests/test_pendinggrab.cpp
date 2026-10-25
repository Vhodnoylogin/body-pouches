#include "Harness.h"
#include "core/PendingGrab.h"

using BodyPouches::Core::PendingGrab;

TEST(unrelated_grab_does_not_confirm_or_cancel_requested_bottle)
{
	PendingGrab pending;
	pending.Arm(0x100001, 200);
	CHECK(!pending.Confirm(0x100002, 100));
	CHECK(pending.Active());
	CHECK(pending.Confirm(0x100001, 199));
	CHECK(!pending.Confirm(0x100001, 199));
}

TEST(late_callback_cannot_confirm_even_before_next_frame_timeout_poll)
{
	for (const auto now : { 200, 201 }) {
		PendingGrab pending;
		pending.Arm(42, 200);
		CHECK(!pending.Confirm(42, now));
		CHECK(pending.Expired(now));
	}
}

TEST(cleared_load_state_rejects_old_grab_and_allows_fresh_handover)
{
	PendingGrab pending;
	pending.Arm(42, 200);
	pending.Clear();
	CHECK(!pending.Active());
	CHECK(!pending.Confirm(42, 100));
	pending.Arm(43, 300);
	CHECK(!pending.Confirm(42, 250));
	CHECK(pending.Confirm(43, 250));
}

TEST(reused_handle_index_with_different_generation_is_not_same_bottle)
{
	PendingGrab pending;
	pending.Arm(0x100001, 200);
	CHECK(!pending.Confirm(0x200001, 100));
	CHECK(pending.Confirm(0x100001, 100));
}
