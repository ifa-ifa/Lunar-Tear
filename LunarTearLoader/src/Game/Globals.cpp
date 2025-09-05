#include "Game/Globals.h"
#include <Windows.h>


uintptr_t g_processBaseAddress;
PhaseScriptManager* phaseScriptManager;
RootScriptManager* rootScriptManager;
GameScriptManager* gameScriptManager;
PlayerSaveData* playerSaveData;
EndingsData* endingsData;
void* localeData;
CPlayerParam* playerParam;
PlayableManager* playableManager;


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

}