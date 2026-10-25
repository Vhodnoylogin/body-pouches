#include "Harness.h"
#include "core/ButtonEdges.h"
using BodyPouches::Core::ButtonEdges;
TEST(controller_edges_require_observed_transition_and_do_not_repeat_held_press)
{
	ButtonEdges b;
	CHECK(b.Sample(true, 1, false) == ButtonEdges::Edge::Reset);
	CHECK(b.Sample(true, 1, true) == ButtonEdges::Edge::Down);
	CHECK(b.Sample(true, 1, true) == ButtonEdges::Edge::None);
	CHECK(b.Sample(true, 1, false) == ButtonEdges::Edge::Up);
}
TEST(controller_edges_do_not_invent_press_on_load_reconnect_or_binding_change)
{
	ButtonEdges b;
	CHECK(b.Sample(true, 1, true) == ButtonEdges::Edge::Reset);
	CHECK(b.Sample(false, 1, false) == ButtonEdges::Edge::Reset);
	CHECK(b.Sample(true, 1, true) == ButtonEdges::Edge::Reset);
	CHECK(b.Sample(true, 2, true) == ButtonEdges::Edge::Reset);
	CHECK(b.Sample(true, 2, false) == ButtonEdges::Edge::Up);
	CHECK(b.Sample(true, 2, true) == ButtonEdges::Edge::Down);
	b.Clear();
	CHECK(b.Sample(true, 2, true) == ButtonEdges::Edge::Reset);
}
