#include "API/api.h"
#include "Common/Logger.h"
#include "Game/Globals.h"
#include "Game/Functions.h"
#include "Common/Settings.h"
#include "Game/d3d11.h"
#include "ModLoader.h"
#include "Hooks/Hooks.h"
#include "VFS/ArchivePatcher.h"
#include "Init.h"
#include "Common/Backup.h"

#include <chrono>

#include <MinHook.h>
#include <tether/tether.h>
#include <replicant/pack.h>
#include <replicant/bxon.h>
#include <replicant/weapon.h>

using enum Logger::LogCategory;

DWORD WINAPI Initialize(LPVOID) {

    try {

        int ret = MH_Initialize();
        if (ret != MH_OK) {
            std::string error_message = std::format("Failed to Initialse Minhook.\n\nError: {}", ret);
            MessageBoxA(NULL, error_message.c_str(), "Lunar Tear", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        InitialiseGlobals();

        InstallVFSHooks(); // Enable this hook ASAP

        try {
            std::filesystem::create_directories("LunarTear/mods");
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::string error_message = std::format("Failed to create mod directories. Please check permissions.\n\nError: {}", e.what());
            MessageBoxA(NULL, error_message.c_str(), "Lunar Tear", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        InitialiseGameFunctions();

        ret = Settings::Instance().LoadFromFile();

        Logger::Init();

        switch (ret) {
        case 0:
            Logger::Log(Verbose) << "Settings loaded successfully from INI file";
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

        InstallTextureHooks();
        InstallScriptInjectHooks();
        InstallTableHooks();
        InstallDebugHooks();
        InstallScriptUpdateHooks();

        // Swapchain and input objects are not initialised until after VFS is initialised
        // Avoid deadlock with VFS hook
        std::jthread fpsunlockthread(InstallFPSUnlockHooks);
        std::jthread enumdevicesthread(InstallEnumDevicesHooks);

        Logger::Log(Verbose) << "Hooks initialized";

        API::Init();

        ScanModsAndResolveConflicts();

        if (Settings::Instance().EnablePlugins) {
            LoadPlugins();
        }

        Logger::Log(Info) << "Lunar Tear initialization complete.";


        if (Settings::Instance().autoBackups) {
            try {
                CreateSaveBackups();
            }
            catch (const std::exception& e) {
                Logger::Log(Error) << "Failed to create save backups: " << e.what();
			}
        }
    }
    
    catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Lunar Tear", MB_OK | MB_ICONERROR);
    }
    catch (...) {
        MessageBoxA(NULL, "An unknown exception occurred during initialization.", "Lunar Tear", MB_OK | MB_ICONERROR);
    }

    return TRUE;

}

