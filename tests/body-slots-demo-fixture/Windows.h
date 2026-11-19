#pragma once
// Explicit DLL-discovery boundary only; no real Windows/provider/game module.
using HMODULE = void*;
using FARPROC = void (*)();
HMODULE GetModuleHandleA(const char*);
FARPROC GetProcAddress(HMODULE, const char*);
