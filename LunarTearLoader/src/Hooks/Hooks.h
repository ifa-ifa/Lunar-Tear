#pragma once
#include <Minhook.h>

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal) {
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

bool InstallTextureHooks();
bool InstallScriptLoadHooks();
bool InstallScriptInjectHooks();
bool InstallScriptUpdateHooks();
bool InstallTableHooks();
bool InstallDebugHooks();
