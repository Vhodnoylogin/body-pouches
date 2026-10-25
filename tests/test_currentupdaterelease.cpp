#include "Harness.h"
#include "core/CurrentUpdateRelease.h"
using BodyPouches::Core::CurrentUpdateRelease;

TEST(shared_drop_event_is_resolved_once_only_in_its_own_update_and_world)
{
	CurrentUpdateRelease release;
	CHECK(!release.Take(2, 10));
	release.Arm(2, 10);
	CHECK(release.Take(2, 10));
	CHECK(!release.Take(2, 10));
	release.Arm(2, 10);
	CHECK(!release.Take(2, 11));
	CHECK(!release.Take(2, 10));  // Rejected events cannot be replayed.
	release.Arm(2, 10);
	CHECK(!release.Take(3, 10)); // Load invalidates even an equal update number.
	CHECK(!release.Active());
}

TEST(shared_drop_event_cancel_or_new_release_cannot_replay_the_old_release)
{
	CurrentUpdateRelease release;
	release.Arm(2, 10);
	release.Clear(); // Regrab, consumption, stash or missing post-update.
	CHECK(!release.Take(2, 10));
	release.Arm(2, 10);
	release.Arm(2, 11);
	CHECK(release.Take(2, 11));
	CHECK(!release.Take(2, 10));
}
