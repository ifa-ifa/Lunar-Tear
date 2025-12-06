#include "Game/Globals.h"
#include <Windows.h>

// TODO: pattern scan these addresses instead of hardcoding them. But this game is not receiving any more updates, so the risk of them changing is low.

uintptr_t g_processBaseAddress;
PhaseScriptManager* phaseScriptManager;
RootScriptManager* rootScriptManager;
GameScriptManager* gameScriptManager;
PlayerSaveData* playerSaveData;
EndingsData* endingsData;
void* localeData;
CPlayerParam* playerParam;
PlayableManager* playableManager;

tpRhiDevice** rhiDevicePtr;
tpRhiSwapchain** rhiSwapchainPtr;

uint8_t* inLoadingScreen;

void* iDirectInput8Ptr;

void InitialiseGlobals() {

	g_processBaseAddress = (uintptr_t)GetModuleHandle(NULL);
	phaseScriptManager = (PhaseScriptManager*)(g_processBaseAddress + 0x48923c0);
	rootScriptManager = (RootScriptManager*)(g_processBaseAddress + 0x484f228);
    gameScriptManager = (GameScriptManager*)(g_processBaseAddress + 0x4870a30);
	playerSaveData = (PlayerSaveData*)(g_processBaseAddress + 0x4374a20);
	endingsData = (EndingsData*)(g_processBaseAddress + 0x11fdf80);
	localeData = (void*)(g_processBaseAddress + 0x27e0590);
	playerParam = (CPlayerParam*)(g_processBaseAddress + 0x122D2C0);
	playableManager = (PlayableManager*)(g_processBaseAddress + 0x26f84b0);
	rhiDevicePtr = (tpRhiDevice**)(g_processBaseAddress + 0x522a5e8);
	rhiSwapchainPtr = (tpRhiSwapchain**)(g_processBaseAddress + 0x522a5f0);

	inLoadingScreen = (uint8_t*)(g_processBaseAddress + 0x4B1C9B4);

	iDirectInput8Ptr = (void*)(g_processBaseAddress + 0x5146e98);

}