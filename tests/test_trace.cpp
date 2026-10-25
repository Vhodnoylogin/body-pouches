#include "Harness.h"
#include "diagnostics/Trace.h"

using namespace BodyPouches::Diagnostics;

TEST(causal_trace_disabled_by_default_never_invents_events)
{
	Trace trace;
	trace.Record({});
	const auto read = trace.Snapshot();
	CHECK(!read.enabled);
	CHECK(!read.available);
	CHECK(read.nextSequence == 1);
	CHECK(read.events.empty());
}

TEST(causal_trace_preserves_exact_handle_deadline_and_world)
{
	Trace trace;
	trace.Enable(true);
	TraceEvent event;
	event.event = Event::PendingArmed;
	event.handle = 0x10123456;
	event.pendingHandle = event.handle;
	event.pendingActive = true;
	event.monotonicMs = 1234;
	event.deadlineMs = 1434;
	event.worldGeneration = 7;
	event.causalId = 12;
	trace.Record(event);
	const auto read = trace.Snapshot();
	CHECK(read.available);
	CHECK(read.events.size() == 1);
	CHECK(read.events[0].sequence == 1);
	CHECK(read.events[0].handle == event.handle);
	CHECK(read.events[0].pendingHandle == event.handle);
	CHECK(read.events[0].deadlineMs - read.events[0].monotonicMs == 200);
	CHECK(read.events[0].worldGeneration == 7);
	CHECK(read.events[0].causalId == 12);
}

TEST(causal_trace_wrap_reports_loss_and_retains_ordered_window)
{
	Trace trace;
	trace.Enable(true);
	for (std::size_t i = 0; i < Trace::kCapacity + 3; ++i) {
		TraceEvent event;
		event.dropSerial = i;
		trace.Record(event);
	}
	const auto read = trace.Snapshot();
	CHECK(read.events.size() == Trace::kCapacity);
	CHECK(read.overwritten == 3);
	CHECK(read.events.front().sequence == 4);
	CHECK(read.events.front().dropSerial == 3);
	CHECK(read.events.back().sequence == Trace::kCapacity + 3);
	CHECK(read.nextSequence == Trace::kCapacity + 4);
	CHECK(read.busyDrops == 0);
}
