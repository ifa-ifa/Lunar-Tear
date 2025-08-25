#include <Windows.h>
#include <MinHook.h>
#include "tether.h"
#include <API/api.h>
#include "Common/Logger.h"
#include "Game/Globals.h"
#include "Game/Functions.h"
#include "Common/Settings.h"
#include "ModLoader.h"
#include "Hooks/Hooks.h"
#include "VFS/ArchivePatcher.h"
#include "VFS/patchInfo.h"


using enum Logger::LogCategory;

static DWORD WINAPI Initialize(LPVOID lpParameter) {

    try {

        int ret = MH_Initialize();
        if (ret != MH_OK) {
            std::string error_message = "Failed to create mod directories. Please check permissions.\n\nError: ";
            error_message += ret;
            MessageBoxA(NULL, error_message.c_str(), "Lunar Tear - Filesystem Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        InitialiseGlobals();

        bool hooks_ok = true;
        hooks_ok &= InstallVFSHooks(); // Enable this hook ASAP

        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            MessageBoxA(NULL, "Could not enable VFS Hook", "Lunar Tear - Filesystem Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        try {
            std::filesystem::create_directories("LunarTear/mods");
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::string error_message = "Failed to create mod directories. Please check permissions.\n\nError: ";
            error_message += e.what();
            MessageBoxA(NULL, error_message.c_str(), "Lunar Tear - Filesystem Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        InitialiseGameFunctions();

        ret = Settings::Instance().LoadFromFile();

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

        hooks_ok &= InstallTextureHooks();
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
        StartArchivePatchingThread();

        StartCacheCleanupThread();

        if (Settings::Instance().EnablePlugins) {
            LoadPlugins();
        }

        Logger::Log(Info) << "Lunar Tear initialization complete.";
    }


    catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Lunar Tear", MB_OK | MB_ICONERROR);
    }
    catch (...) {
        MessageBoxA(NULL, "An unknown exception occurred during initialization.", "Lunar Tear", MB_OK | MB_ICONERROR);
    }

    return TRUE;

}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, Initialize, hModule, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        StopArchivePatchingThread();
        StopCacheCleanupThread();
        MH_Uninitialize();
    }
    return TRUE;
}