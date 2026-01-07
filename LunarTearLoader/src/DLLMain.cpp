#include <Windows.h>
#include <filesystem>
#include <format>
#include "Init.h"
#include <fstream>
#include "ModLoader.h"
#include <crc32c/crc32c.h>
#include <MinHook.h>

namespace {

    typedef uint64_t(__fastcall* tLoadArchives)(uint64_t, uint64_t, uint64_t, uint64_t);
    tLoadArchives loadArchives_original = nullptr;
    uint64_t loadArchives_detour(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
        int slept = 0;
        while(!g_lunarTearInitialised) {
            Sleep(100);
		    slept += 100;
            if (slept > 10000) {
                MessageBoxW(NULL, L"Lunar Tear failed to initialize.", L"Lunar Tear", MB_OK);
                break;
		    }
	    }
        return loadArchives_original(arg1, arg2, arg3, arg4);
    }

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        
        int ret = MH_Initialize();
        if (ret != MH_OK) {
            std::string error_message = std::format("Failed to Initialse Minhook.\n\nError: {}", ret);
            MessageBoxA(NULL, error_message.c_str(), "Lunar Tear", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        void* loadArchives_target = reinterpret_cast<uint8_t*>(GetModuleHandle(NULL)) + 0x8ed1f0;

		ret = MH_CreateHook(loadArchives_target, loadArchives_detour, reinterpret_cast<LPVOID*>( & loadArchives_original));
        if (ret != MH_OK) {
			MessageBoxW(NULL, std::format(L"Failed to create hook for loadArchives: {}", ret).c_str(), L"Lunar Tear", MB_OK);
        }
		ret = MH_EnableHook(loadArchives_target);
		if (ret != MH_OK) {
			MessageBoxW(NULL, std::format(L"Failed to enable hook for loadArchives: {}", ret).c_str(), L"Lunar Tear", MB_OK);
		}



        DisableThreadLibraryCalls(hModule);

        wchar_t processPath[MAX_PATH];
        GetModuleFileNameW(NULL, processPath, MAX_PATH);

        std::ifstream file(processPath, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);

        if (file.read(buffer.data(), size)) {
            uint32_t currentHash = crc32c::Crc32c(buffer.data(), size);
            const uint32_t expectedHash = 396277532;

            if (currentHash != expectedHash) {
                int result = MessageBoxW(
                    NULL,
                    std::format(L"CRC32c mismatch - Expected: {:#x}, Actual: {:#x}\n\nYour game executable may be modified, corrupted, or on the wrong game version. Expect Lunar Tear to not work properly. Continue anyway?", expectedHash, currentHash).c_str(),
                    L"Lunar Tear",
                    MB_OKCANCEL
                );
                if (result != IDOK) {

                    return FALSE;
                }
            }
        }
        
        HANDLE hThread = CreateThread(nullptr, 0, Initialize, hModule, 0, nullptr);
   

        if (hThread) CloseHandle(hThread);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {

        //MH_Uninitialize(); // Causes deadlock
    }
    return TRUE;
}