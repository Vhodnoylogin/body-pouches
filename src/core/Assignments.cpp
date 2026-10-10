#include "core/Assignments.h"

#include <set>

namespace BodyPouches::Core
{
	namespace
	{
		bool KeyValid(const FormKey& a_key, bool a_allowEmpty = false)
		{
			if (!a_key.IsSet()) { return a_allowEmpty && a_key.plugin.empty(); }
			if (a_key.plugin.empty()) { return a_key.localId >= 0xFF000000; }
			return a_key.plugin.size() <= 255 && a_key.localId <= 0xFFFFFF &&
				a_key.plugin.find('\0') == std::string::npos;
		}
		void Put(std::vector<std::uint8_t>& a_bytes, std::uint32_t a_value)
		{
			for (int shift = 0; shift < 32; shift += 8) {
				a_bytes.push_back(static_cast<std::uint8_t>(a_value >> shift));
			}
		}
		void PutKey(std::vector<std::uint8_t>& a_bytes, const FormKey& a_key)
		{
			Put(a_bytes, a_key.localId);
			Put(a_bytes, static_cast<std::uint32_t>(a_key.plugin.size()));
			a_bytes.insert(a_bytes.end(), a_key.plugin.begin(), a_key.plugin.end());
		}
		struct Reader
		{
			std::span<const std::uint8_t> bytes;
			std::size_t at{};
			bool Get(std::uint32_t& a_value)
			{
				if (bytes.size() - at < 4) { return false; }
				a_value = 0;
				for (int shift = 0; shift < 32; shift += 8) { a_value |= std::uint32_t(bytes[at++]) << shift; }
				return true;
			}
			bool Key(FormKey& a_key, bool a_allowEmpty = false)
			{
				std::uint32_t length{};
				if (!Get(a_key.localId) || !Get(length) || length > 255 || length > bytes.size() - at) { return false; }
				a_key.plugin.assign(reinterpret_cast<const char*>(bytes.data() + at), length);
				at += length;
				return KeyValid(a_key, a_allowEmpty);
			}
		};
	}

	Assignments CaptureAssignments(const PouchSet& a_pouches)
	{
		Assignments result;
		for (const auto& state : a_pouches.Snapshot()) {
			if (const auto* pouch = a_pouches.Find(state.slot); pouch && pouch->IsAssigned()) {
				result.push_back({ state.slot, pouch->PouchFilter() });
			}
		}
		return result;
	}

	void RestoreAssignments(PouchSet& a_pouches, const Assignments& a_saved)
	{
		for (const auto& state : a_pouches.Snapshot()) { a_pouches.Find(state.slot)->Forget(); }
		a_pouches.NoteSettled(true);
		a_pouches.NoteSettled(false);
		for (const auto& saved : a_saved) {
			if (auto* pouch = a_pouches.Find(saved.slot)) { pouch->Assign(saved.filter); }
		}
	}

	std::optional<std::vector<std::uint8_t>> EncodeAssignments(const Assignments& a_saved)
	{
		if (a_saved.size() > 14) { return std::nullopt; }
		std::set<int> slots;
		std::vector<std::uint8_t> bytes;
		Put(bytes, static_cast<std::uint32_t>(a_saved.size()));
		for (const auto& saved : a_saved) {
			const auto& filter = saved.filter;
			if (saved.slot < 1 || saved.slot > 14 || !slots.insert(saved.slot).second ||
				!filter.IsSet() || !KeyValid(filter.exact, true) || filter.effects.size() > 64 ||
				(filter.exact.IsSet() && !filter.effects.empty())) { return std::nullopt; }
			Put(bytes, static_cast<std::uint32_t>(saved.slot));
			PutKey(bytes, filter.exact);
			Put(bytes, static_cast<std::uint32_t>(filter.effects.size()));
			for (const auto& effect : filter.effects) {
				if (!KeyValid(effect)) { return std::nullopt; }
				PutKey(bytes, effect);
			}
		}
		if (bytes.size() > kAssignmentBytes) { return std::nullopt; }
		return bytes;
	}

	std::optional<Assignments> DecodeAssignments(std::span<const std::uint8_t> a_bytes)
	{
		if (a_bytes.size() > kAssignmentBytes) { return std::nullopt; }
		Reader reader{ a_bytes };
		std::uint32_t count{};
		if (!reader.Get(count) || count > 14) { return std::nullopt; }
		Assignments result;
		std::set<int> slots;
		for (std::uint32_t i = 0; i < count; ++i) {
			std::uint32_t slot{}, effects{};
			Filter filter;
			if (!reader.Get(slot) || slot < 1 || slot > 14 || !slots.insert(static_cast<int>(slot)).second ||
				!reader.Key(filter.exact, true) || !reader.Get(effects) || effects > 64 ||
				(filter.exact.IsSet() && effects)) { return std::nullopt; }
			for (std::uint32_t j = 0; j < effects; ++j) {
				FormKey key;
				if (!reader.Key(key)) { return std::nullopt; }
				filter.effects.push_back(std::move(key));
			}
			if (!filter.IsSet()) { return std::nullopt; }
			result.push_back({ static_cast<int>(slot), std::move(filter) });
		}
		if (reader.at != a_bytes.size()) { return std::nullopt; }
		return result;
	}

	Assignments ResolveAssignments(const Assignments& a_saved,
		const std::function<std::optional<FormKey>(const FormKey&, bool)>& a_resolve)
	{
		Assignments result;
		for (const auto& saved : a_saved) {
			Filter filter;
			bool valid = true;
			if (saved.filter.exact.IsSet()) {
				auto key = a_resolve(saved.filter.exact, false);
				if (!key || !key->IsSet()) { valid = false; }
				else { filter.exact = std::move(*key); }
			}
			for (const auto& effect : saved.filter.effects) {
				auto key = a_resolve(effect, true);
				if (!key || !key->IsSet()) { valid = false; break; }
				filter.effects.push_back(std::move(*key));
			}
			if (valid && filter.IsSet()) { result.push_back({ saved.slot, std::move(filter) }); }
		}
		return result;
	}

	std::optional<Assignments> ReadAssignmentRecords(AssignmentRecords& a_reader)
	{
		std::uint32_t type{}, version{}, length{};
		bool seen = false;
		bool invalid = false;
		Assignments saved;
		while (a_reader.Next(type, version, length)) {
			if (type != kAssignmentRecord) { continue; }
			if (seen || version != kAssignmentVersion || length > kAssignmentBytes) {
				invalid = true;
				seen = true;
				continue;
			}
			seen = true;
			std::vector<std::uint8_t> bytes(length);
			if (a_reader.Read(bytes) != length) { invalid = true; continue; }
			auto decoded = DecodeAssignments(bytes);
			if (!decoded) { invalid = true; continue; }
			saved = std::move(*decoded);
		}
		return invalid ? std::nullopt : std::optional<Assignments>(std::move(saved));
	}
}
