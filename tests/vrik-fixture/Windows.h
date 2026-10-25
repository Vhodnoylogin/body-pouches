#pragma once
// Loader boundary double only. No real module is loaded by these tests.
using HMODULE = void*;
using FARPROC = void (*)();
inline FARPROC fixtureSuspensionQuery{};
inline HMODULE GetModuleHandleA(const char*) { return fixtureSuspensionQuery ? reinterpret_cast<void*>(1) : nullptr; }
inline FARPROC GetProcAddress(HMODULE, const char*) { return fixtureSuspensionQuery; }
