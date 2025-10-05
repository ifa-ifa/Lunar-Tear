#include <Windows.h>
#include "ModLoader.h"
#include <timeapi.h> 
#pragma comment(lib, "winmm.lib") 
#include "Init.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {

        timeBeginPeriod(1);

        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, Initialize, hModule, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        StopCacheCleanupThread();

        timeEndPeriod(1); 

        //MH_Uninitialize(); // Causes deadlock
    }
    return TRUE;
}