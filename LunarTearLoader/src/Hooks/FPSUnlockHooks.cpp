#include "Hooks/Hooks.h"
#include "Common/Logger.h"
#include "Game/Globals.h"
#include "Game/d3d11.h"
#include <Windows.h>
#include <vector>
#include <dxgi.h>

using enum Logger::LogCategory;

namespace FPSUnlock {

    bool WriteMemory(uintptr_t address, const std::vector<uint8_t>& bytes) {
        DWORD oldProtect;
        if (VirtualProtect((LPVOID)address, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy((LPVOID)address, bytes.data(), bytes.size());
            VirtualProtect((LPVOID)address, bytes.size(), oldProtect, &oldProtect);
            return true;
        }
        return false;
    }

    typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    Present_t oPresent = nullptr;

    uint8_t* bLoadScreen = nullptr;

    HRESULT WINAPI Present_Detour(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {

        if (bLoadScreen != nullptr && *bLoadScreen != 0) {
            return oPresent(pSwapChain, 1, Flags);
        }

        return oPresent(pSwapChain, 0, Flags);
    }
}

bool InstallFPSUnlockHooks() {

    uintptr_t dtFixAddr = g_processBaseAddress + 0x6938B;
    std::vector<uint8_t> dtFixPatch = { 0x81 };

    uintptr_t rotationDetourAddr = g_processBaseAddress + 0x66E580;
    std::vector<uint8_t> rotationDetourPatch = { 0xE9, 0x6B, 0x8D, 0xE0, 0xFF, 0x90, 0x90, 0x90 };

    uintptr_t rotationCaveAddr = g_processBaseAddress + 0x4772F0;
    std::vector<uint8_t> rotationCavePatch = {
        0xF3, 0x0F, 0x10, 0x0D, 0xBC, 0x7D, 0x8D, 0x00, 0xF3, 0x0F, 0x5E, 0x0D,
        0x40, 0x60, 0x6A, 0x04, 0xF3, 0x0F, 0x59, 0xD1, 0xE9, 0x7F, 0x72, 0x1F,
        0x00, 0x90
    };

    bool success = true;
    success &= FPSUnlock::WriteMemory(dtFixAddr, dtFixPatch);
    success &= FPSUnlock::WriteMemory(rotationDetourAddr, rotationDetourPatch);
    success &= FPSUnlock::WriteMemory(rotationCaveAddr, rotationCavePatch);

    if (success) {
        Logger::Log(Verbose) << "Successfully applied FPS patches";
    }
    else {
        Logger::Log(Error) << "Failed to apply FPS patches";
        return false;
    }

    FPSUnlock::bLoadScreen = (uint8_t*)(g_processBaseAddress + 0x4B1C9B4);

    void* pPresent = GetPresentAddress();
    if (!pPresent) {
        Logger::Log(Error) << "Could not get Present address. FPS limiter for loading screens will not work";
    }
    else
    {
        if (MH_CreateHookEx(pPresent, &FPSUnlock::Present_Detour, &FPSUnlock::oPresent) != MH_OK) {
            Logger::Log(Error) << "Could not create hook for IDXGISwapChain::Present.";
            return false;
        }
    }

    Logger::Log(Info) << "FPS unlock hooks installed successfully.";
    return true;
}