#include "diagnostics/Inspect.h"
#include "diagnostics/Contract.h"
#include "DevBenchAPI.h"
#include "Mod.h"

#include <Windows.h>
#include <chrono>
#include <cstring>
#include <future>
#include <memory>
#include <stdexcept>

namespace BodyPouches::Diagnostics
{
	namespace
	{
		using json = nlohmann::json;
		using Clock = std::chrono::steady_clock;
		std::atomic<std::uint64_t> epoch{ 0 }, sequence{ 0 };
		std::atomic<unsigned> pending{ 0 };
		std::atomic<bool> loading{ false }, failedLoad{ false };
		constexpr const char* kKind = "body_pouches_state";

		std::int64_t Now()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
		}

		json Envelope()
		{
			FILETIME birth{}, exit{}, kernel{}, user{};
			const bool identified = GetProcessTimes(GetCurrentProcess(), &birth, &exit, &kernel, &user) != FALSE;
			const auto created = (std::uint64_t(birth.dwHighDateTime) << 32) | birth.dwLowDateTime;
			return { { "schemaVersion", 1 }, { "provider", kKind }, { "providerVersion", PLUGIN_VERSION },
				{ "process", { { "pid", GetCurrentProcessId() }, { "identityAvailable", identified },
					{ "createdFileTime100ns", identified ? json(created) : json(nullptr) } } },
				{ "lifecycleEpoch", epoch.load() }, { "readOnly", true } };
		}

		json Error(const char* a_reason)
		{
			auto result = Envelope();
			result["ok"] = false;
			result["available"] = false;
			result["reason"] = a_reason;
			return result;
		}

		struct Sample
		{
			State state;
			std::uint64_t sequence{};
			std::int64_t monotonicNs{};
		};
		struct Job
		{
			Job(std::int64_t a_deadline, std::uint64_t a_epoch) : gate(a_deadline, a_epoch), epoch(a_epoch) {}
			ReadGate gate;
			std::uint64_t epoch;
			std::promise<Sample> promise;
		};

		json Read(unsigned a_timeoutMs)
		{
			auto* tasks = SKSE::GetTaskInterface();
			if (tasks == nullptr) { return Error("main_thread_tasks_unavailable"); }
			if (pending.fetch_add(1) >= 2) {
				--pending;
				return Error("read_capacity_exceeded");
			}
			struct Reservation {
				bool transferred{ false };
				~Reservation() { if (!transferred) { --pending; } }
			} reservation;
			auto job = std::make_shared<Job>(Now() + std::int64_t(a_timeoutMs) * 1000000, epoch.load());
			auto future = job->promise.get_future();
			tasks->AddTask([job]() {
				struct Release { ~Release() { --pending; } } release;
				Sample value;
				try {
					if (!job->gate.Begin(Now(), epoch.load()) || loading.load() || failedLoad.load()) {
						value.state.reason = "expired_loading_or_generation_changed";
					} else {
						value.state = Mod::GetSingleton().ReadState();
						value.monotonicNs = Now();
						value.sequence = ++sequence;
						if (!job->gate.Valid(Now(), epoch.load())) {
							value.state = {};
							value.state.reason = "read_expired_or_generation_changed";
						}
					}
					job->promise.set_value(std::move(value));
				} catch (...) { job->promise.set_exception(std::current_exception()); }
			});
			reservation.transferred = true;
			const auto deadline = Clock::time_point(std::chrono::duration_cast<Clock::duration>(
				std::chrono::nanoseconds(job->gate.Deadline())));
			if (future.wait_until(deadline) != std::future_status::ready) {
				job->gate.Cancel();
				return Error("main_thread_read_timed_out");
			}
			const auto sample = future.get();
			if (!job->gate.Valid(Now(), epoch.load()) || loading.load() || failedLoad.load()) {
				return Error("read_expired_loading_or_generation_changed");
			}
			auto result = Envelope();
			result["ok"] = sample.state.available;
			result["available"] = sample.state.available;
			result["subject"] = StateValues(sample.state);
			result["sample"] = { { "sequence", sample.sequence }, { "producerMonotonicNs", sample.monotonicNs },
				{ "clock", "std::chrono::steady_clock nanoseconds; process-local epoch" },
				{ "phase", "skse_main_thread_task" }, { "lifecycleEpoch", job->epoch } };
			if (!result["process"]["identityAvailable"].get<bool>()) { return Error("process_identity_unavailable"); }
			return result;
		}

		void Handle(void*, const char* a_text, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			if (a_write == nullptr) { return; }
			json result;
			try {
				if (a_text == nullptr || strnlen_s(a_text, 1025) > 1024) { throw std::invalid_argument("request_exceeds_1024_bytes"); }
				const auto args = json::parse(a_text);
				if (!args.is_object()) { throw std::invalid_argument("request_must_be_an_object"); }
				for (const auto& entry : args.items()) {
					if (entry.key() != "kind" && entry.key() != "action" && entry.key() != "timeoutMs") {
						throw std::invalid_argument("unsupported_request_field");
					}
				}
				if (args.value("kind", std::string(kKind)) != kKind) { throw std::invalid_argument("unsupported_kind"); }
				const auto action = args.value("action", std::string("snapshot"));
				if (action == "capabilities") {
					result = Envelope();
					result["ok"] = true;
					result["actions"] = { "snapshot", "capabilities" };
					result["bounds"] = { { "requestBytes", 1024 }, { "pendingTasks", 2 },
						{ "timeoutMs", { 100, 3000 } }, { "pouchSlots", 14 } };
				} else if (action == "snapshot") {
					const auto timeout = args.value("timeoutMs", json(1000));
					if (!timeout.is_number_integer() || timeout.get<std::int64_t>() < 100 || timeout.get<std::int64_t>() > 3000) {
						throw std::invalid_argument("timeoutMs_must_be_integer_100_to_3000");
					}
					result = Read(timeout.get<unsigned>());
				} else { throw std::invalid_argument("unsupported_action"); }
			} catch (const std::exception& error) { result = Error(error.what()); }
			std::string output;
			try { output = result.dump(); }
			catch (const std::exception&) { output = Error("state_serialization_failed").dump(); }
			a_write(a_sink, output.c_str());
		}
	}

	void Invalidate() noexcept { ++epoch; loading.store(true); }
	void SetLoadResult(bool a_succeeded) noexcept { failedLoad.store(!a_succeeded); loading.store(!a_succeeded); }

	void Connect()
	{
		try {
			auto* api = DevBenchAPI::GetDevBenchInterface001();
			if (api == nullptr || api->GetBuildNumber() < 10500) { return; }
			json properties;
			properties["kind"] = { { "const", kKind } };
			properties["action"] = { { "enum", { "snapshot", "capabilities" } } };
			properties["timeoutMs"] = { { "type", "integer" }, { "minimum", 100 }, { "maximum", 3000 } };
			const json descriptor{
				{ "description", "Read effective Body Pouches settings and runtime assignment count; no mutation" },
				{ "readOnly", true },
				{ "inputSchema", { { "type", "object" }, { "additionalProperties", false }, { "properties", properties } } } };
			api->RegisterToolExtension("inspect", kKind, descriptor.dump().c_str(), Handle, nullptr);
		} catch (const std::exception& error) {
			spdlog::warn("Optional state inspection unavailable: {}", error.what());
		}
	}
}
