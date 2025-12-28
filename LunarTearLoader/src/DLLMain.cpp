#include <Windows.h>
#include <filesystem>
#include <format>
#include "Init.h"
#include <fstream>
#include "ModLoader.h"
#include <crc32c/crc32c.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        
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