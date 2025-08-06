#include <Windows.h>
#include <MinHook.h>
#include <API/api.h>
#include "Common/Logger.h"
#include "Game/Globals.h"
#include "Game/Functions.h"
#include "Common/Settings.h"
#include "ModLoader.h"
#include "Hooks/Hooks.h"

using enum Logger::LogCategory;

static DWORD WINAPI Initialize(LPVOID lpParameter) {

    InitialiseGlobals();
    InitialiseGameFunctions();

    int ret = Settings::Instance().LoadFromFile();

    Logger::Init();

    switch (ret) {
    case 0:
        Logger::Log(Info) << "Settings loaded successfully from INI file";
        break;
    case 1:
        Logger::Log(Warning) << "INI file exists but is corrupt. Using default settings for this session. Correct the error, or delete the ini file to regenerate a new one on restart";
        break;
    case 2:
        Logger::Log(Info) << "INI file not found. A new one has been created with default settings";
        break;
    }
    if (!g_processBaseAddress) {
        Logger::Log(Error) << "Could not get process base address. Cannot initialize Lunar Tear.";
        return FALSE;
    }

    if (MH_Initialize() != MH_OK) {
        Logger::Log(Error) << "Could not initialize MinHook.";
        return FALSE;
    }

    bool hooks_ok = true;
    hooks_ok &= InstallTextureHooks();
    hooks_ok &= InstallScriptLoadHooks();
    hooks_ok &= InstallScriptInjectHooks();
    hooks_ok &= InstallTableHooks();
    hooks_ok &= InstallDebugHooks();
    hooks_ok &= InstallScriptUpdateHooks();

    
    if (!hooks_ok) {
        Logger::Log(Warning) << "Failed to initialise hooks. Mod loader may not work correctly";
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        Logger::Log(Error) << "Could not enable hooks";
        return FALSE;
    }

    Logger::Log(Verbose) << "Hooks initialized";


    API::Init();

    
    ScanModsAndResolveConflicts();
    StartCacheCleanupThread();

    if (Settings::Instance().EnablePlugins) {
        LoadPlugins();
    }

    Logger::Log(Info) << "Lunar Tear initialization complete.";

    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, Initialize, hModule, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        StopCacheCleanupThread();
        MH_Uninitialize();
    }
    return TRUE;
}