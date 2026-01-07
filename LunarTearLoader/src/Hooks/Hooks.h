#pragma once
#include <Minhook.h>

#include <Common/Logger.h>

using enum Logger::LogCategory;

template <typename T>
inline bool InstallHook(LPVOID pTarget, LPVOID pDetour, T** ppOriginal, std::string name) {
    int ret;
	ret = MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
    if (ret != MH_OK) {
        Logger::Log(Error) << "Could not create hook for " << name << ": " << ret;
        return false;
    }
	ret = MH_EnableHook(pTarget);
    if (ret != MH_OK) {
        Logger::Log(Error) << "Could not enable hook for " << name << ": " << ret;
        return false;
    }
	return true;
}


bool InstallTextureHooks();
bool InstallScriptInjectHooks();
bool InstallScriptUpdateHooks();
bool InstallTableHooks();
bool InstallDebugHooks();
bool InstallVFSHooks();
bool InstallFPSUnlockHooks();
bool InstallEnumDevicesHooks();

