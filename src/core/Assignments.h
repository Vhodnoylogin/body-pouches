#pragma once

#include "core/PouchSet.h"
#include <functional>
#include <span>

namespace BodyPouches::Core
{
	struct Assignment
	{
		int slot{};
		Filter filter;
	};
	using Assignments = std::vector<Assignment>;
	inline constexpr std::size_t kAssignmentBytes = 256 * 1024;
	inline constexpr std::uint32_t kAssignmentRecord = 0x4153474E;
	inline constexpr std::uint32_t kAssignmentVersion = 1;
	class AssignmentRecords
	{
	public:
		virtual ~AssignmentRecords() = default;
		virtual bool Next(std::uint32_t& a_type, std::uint32_t& a_version, std::uint32_t& a_length) = 0;
		virtual std::uint32_t Read(std::span<std::uint8_t> a_bytes) = 0;
	};
	[[nodiscard]] std::optional<Assignments> ReadAssignmentRecords(AssignmentRecords& a_reader);
	[[nodiscard]] Assignments CaptureAssignments(const PouchSet& a_pouches);
	// Restoring a save replaces ALL assignments, including when the save has none.
	// Current configured slots/modes win; inventory and in-flight hands are not saved.
	void RestoreAssignments(PouchSet& a_pouches, const Assignments& a_saved);
	[[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeAssignments(const Assignments& a_saved);
	[[nodiscard]] std::optional<Assignments> DecodeAssignments(std::span<const std::uint8_t> a_bytes);
	// Missing any form rejects the complete filter, never broadens a multi-effect one.
	[[nodiscard]] Assignments ResolveAssignments(const Assignments& a_saved,
		const std::function<std::optional<FormKey>(const FormKey&, bool)>& a_resolve);
}
