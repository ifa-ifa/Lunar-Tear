#pragma once
#include <Minhook.h>
#include <chrono> 

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal) {
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

bool InstallTextureHooks();
bool InstallScriptInjectHooks();
bool InstallScriptUpdateHooks();
bool InstallTableHooks();
bool InstallDebugHooks();
bool InstallVFSHooks();
bool InstallFPSUnlockHooks();
bool InstallEnumDevicesHooks();

