#pragma once
#include <Game/Types.h>


extern uintptr_t g_processBaseAddress;
extern PhaseScriptManager* phaseScriptManager; // __libnier__ script is also handled by the phaseScriptManager
extern RootScriptManager* rootScriptManager;
extern GameScriptManager* gameScriptManager;
extern EndingsData* endingsData;
extern void* localeData;
extern CPlayerParam* playerParam;

extern PlayableManager* playableManager;

extern PlayerSaveData* playerSaveData;

extern tpRhiDevice** rhiDevicePtr;
extern tpRhiSwapchain** rhiSwapchainPtr;

extern uint8_t* inLoadingScreen; // 0 = false, !0 = true

void InitialiseGlobals();