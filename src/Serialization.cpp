#include "Serialization.h"
#include "Mod.h"
#include "core/Assignments.h"

#include <SKSE/SKSE.h>
#include <spdlog/spdlog.h>

namespace BodyPouches::Serialization
{
	namespace
	{
		constexpr std::uint32_t kUniqueID = 0x42504348; // BPCH
		class Reader final : public Core::AssignmentRecords
		{
		public:
			explicit Reader(SKSE::SerializationInterface& a_skse) : _skse(a_skse) {}
			bool Next(std::uint32_t& type, std::uint32_t& version, std::uint32_t& length) override
			{
				return _skse.GetNextRecordInfo(type, version, length);
			}
			std::uint32_t Read(std::span<std::uint8_t> bytes) override
			{
				return _skse.ReadRecordData(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
			}
		private:
			SKSE::SerializationInterface& _skse;
		};
		void Save(SKSE::SerializationInterface* a_skse)
		{
			const auto saved = Mod::GetSingleton().SavedAssignments();
			const auto bytes = Core::EncodeAssignments(saved);
			if (!a_skse || !bytes || !a_skse->WriteRecord(Core::kAssignmentRecord, Core::kAssignmentVersion, bytes->data(), static_cast<std::uint32_t>(bytes->size()))) {
				spdlog::error("persistence.save_failed: assignment record was not written");
				return;
			}
			spdlog::info("persistence.saved: {} assignments, {} bytes", saved.size(), bytes->size());
		}
		void Load(SKSE::SerializationInterface* a_skse)
		{
			// Load may be called without a preceding record for this plugin.
			Mod::GetSingleton().SetLoadedAssignments({});
			if (!a_skse) { return; }
			Reader reader(*a_skse);
			const auto decoded = Core::ReadAssignmentRecords(reader);
			if (!decoded) {
				spdlog::warn("persistence.load_rejected: invalid, unsupported or duplicate assignment record; pouches unassigned");
				return;
			}
			auto saved = Core::ResolveAssignments(*decoded, [a_skse](const Core::FormKey& key, bool) -> std::optional<Core::FormKey> {
					if (!key.plugin.empty()) { return key; }
					std::uint32_t remapped{};
					if (!a_skse->ResolveFormID(key.localId, remapped) || !remapped) { return std::nullopt; }
					return Core::FormKey{ "", remapped };
			});
			spdlog::info("persistence.loaded: {} filters decoded, {} IDs resolved", decoded->size(), saved.size());
			Mod::GetSingleton().SetLoadedAssignments(std::move(saved));
		}
		void Revert(SKSE::SerializationInterface*)
		{
			Mod::GetSingleton().OnGameRevert();
			spdlog::info("persistence.reverted: assignments and queued hand work cleared");
		}
	}

	bool Register()
	{
		auto* skse = SKSE::GetSerializationInterface();
		if (!skse) { spdlog::error("persistence.unavailable: SKSE serialization required"); return false; }
		skse->SetUniqueID(kUniqueID);
		skse->SetSaveCallback(Save);
		skse->SetLoadCallback(Load);
		skse->SetRevertCallback(Revert);
		return true;
	}
}
