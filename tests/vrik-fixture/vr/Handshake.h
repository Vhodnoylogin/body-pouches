#pragma once
#include <cstdint>
namespace BodyPouches::VR {
inline void* fixtureVrikApi = nullptr;
template<class T, std::uint32_t> T* AskFor(const char*) {
    return static_cast<T*>(fixtureVrikApi);
}
}
