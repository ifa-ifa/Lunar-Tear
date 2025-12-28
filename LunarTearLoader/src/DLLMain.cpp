#include <Windows.h>
#include "ModLoader.h"
#include <timeapi.h> 
#include "Common/SHA256.h"
#include <filesystem>
#include <format>
#pragma comment(lib, "winmm.lib") 
#include "Init.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        
        DisableThreadLibraryCalls(hModule);

        wchar_t processPath[MAX_PATH];
        GetModuleFileNameW(NULL, processPath, MAX_PATH);

        std::vector<unsigned char> currentHash;
        if (!HashFileSHA256(processPath, currentHash)) {
            MessageBoxW(NULL, L"Fatal error - HashFileSHA256 Failed", L"Lunar Tear", MB_OK);
            return FALSE;
        }

        static const unsigned char expectedHash[32] = {
            0x90,0x76,0x93,0xCB,0x5E,0x03,0xC6,0x1D,0xD4,0x22,0xE8,0xEE,0xAD,0xF1,0xCE,0x73,
            0xF3,0x12,0x2A,0x6C,0xA4,0xE5,0x07,0x99,0xBF,0xC7,0x36,0xBF,0x83,0x9E,0xED,0x16
        };

        if (memcmp(expectedHash, currentHash.data(), 32) != 0) {
            std::wstring expectedHex = BytesToHex(expectedHash, 32);
            std::wstring actualHex = BytesToHex(currentHash.data(), currentHash.size());

            int result = MessageBoxW(
                NULL,
                std::format(L"SHA256 mismatch - Expected: {}, Actual: {}\n\nYour game executable may be modified, corrupted, or on the wrong game version. Expect Lunar Tear to not work properly. Continue anyway?", expectedHex, actualHex).c_str(),
                L"Lunar Tear",
                MB_OKCANCEL
            );
            if (result != IDOK) {

                return FALSE;
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