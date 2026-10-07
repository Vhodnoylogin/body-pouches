#pragma once

// Narrow engine boundary fixture for compiling the real game/Forms.cpp without
// Skyrim. GetLocalFormID has CommonLibSSE-NG 3.7.0's source-file precondition;
// throw instead of dereferencing null so a regression reports an ordinary failure.
// This does not model engine alchemy, saves, native reference ownership or VR.
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace RE
{
    struct TESFile
    {
        std::string name;
        std::uint8_t compileIndex{};
        std::uint16_t smallFileCompileIndex{};
        std::string_view GetFilename() const { return name; }
    };

    struct TESForm
    {
        std::uint32_t id{};
        TESFile* file{};
        static inline std::map<std::uint32_t, TESForm*> runtime;
        TESFile* GetFile(int) const { return file; }
        std::uint32_t GetFormID() const { return id; }
        bool IsDynamicForm() const { return id >= 0xFF000000; }
        std::uint32_t GetLocalFormID() const
        {
            if (!file) throw std::logic_error("GetLocalFormID requires a source file");
            return id & ~((std::uint32_t(file->compileIndex) << 24) +
                          (std::uint32_t(file->smallFileCompileIndex) << 12));
        }
        static TESForm* LookupByID(std::uint32_t id)
        {
            auto it = runtime.find(id);
            return it == runtime.end() ? nullptr : it->second;
        }
    };

    struct Effect
    {
        TESForm* baseEffect{};
        float magnitude{};
        float GetMagnitude() const { return magnitude; }
    };
    struct AlchemyItem : TESForm
    {
        bool poison{};
        std::vector<Effect*> effects;
        bool IsPoison() { return poison; }
    };

    struct TESDataHandler
    {
        static inline TESDataHandler* instance{};
        std::map<std::pair<std::uint32_t, std::string>, TESForm*> forms;
        static TESDataHandler* GetSingleton() { return instance; }
        TESForm* LookupForm(std::uint32_t id, const std::string& plugin)
        {
            auto it = forms.find({id, plugin});
            return it == forms.end() ? nullptr : it->second;
        }
    };
}
