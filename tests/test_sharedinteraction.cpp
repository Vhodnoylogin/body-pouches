#include "Harness.h"
#include "core/SharedInteraction.h"
#include <limits>

using BodyPouches::Core::SharedWithdrawal;

TEST(shared_requires_press_inside_then_withdrawal_and_fires_only_once)
{
	SharedWithdrawal gesture;
	CHECK_EQ(gesture.Update(true, 20, 10, 1), 0);
	gesture.Press(13, 20, 10, 2);
	CHECK_EQ(gesture.Update(true, 1, 10, 3), 0);
	CHECK_EQ(gesture.Update(true, 20, 10, 4), 0);
	gesture.Press(13, 1, 10, 5);
	CHECK_EQ(gesture.Update(true, 10, 10, 6), 0);
	CHECK_EQ(gesture.Update(true, 10.01, 10, 7), 13);
	CHECK_EQ(gesture.Update(true, 20, 10, 8), 0);
}

TEST(shared_release_native_ownership_or_occupied_hand_cancels_without_rearming)
{
	SharedWithdrawal gesture;
	gesture.Press(14, 1, 10, 0);
	gesture.Clear();
	CHECK_EQ(gesture.Update(true, 20, 10, 1), 0);
	gesture.Press(14, 1, 10, 2);
	CHECK_EQ(gesture.Update(false, 1, 10, 3), 0);
	CHECK_EQ(gesture.Update(true, 20, 10, 4), 0);
	gesture.Press(14, 1, 10, 5);
	CHECK_EQ(gesture.Update(true, 20, 10, 6), 14);
}

TEST(shared_invalid_geometry_timeout_and_load_reset_cannot_draw)
{
	SharedWithdrawal gesture;
	for (const double invalid : { -1., std::numeric_limits<double>::infinity(),
		std::numeric_limits<double>::quiet_NaN() }) {
		gesture.Press(13, invalid, 10, 0);
		CHECK_EQ(gesture.Slot(), 0);
		gesture.Press(13, 1, 10, 0);
		CHECK_EQ(gesture.Update(true, invalid, 10, 1), 0);
		CHECK_EQ(gesture.Update(true, 20, 10, 2), 0);
	}
	gesture.Press(13, 1, 10, 0);
	CHECK_EQ(gesture.Update(true, 20, 10, 3000), 0);
	gesture.Press(13, 1, 10, 10);
	CHECK_EQ(gesture.Update(true, 20, 10, 9), 0);
	gesture.Press(13, 1, 10, 10);
	gesture.Clear();
	CHECK_EQ(gesture.Update(true, 20, 10, 11), 0);
}
