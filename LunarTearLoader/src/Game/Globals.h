#pragma once
#include <Game/Types.h>


extern uintptr_t g_processBaseAddress;
extern PhaseScriptManager* phaseScriptManager; 
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

extern void* iDirectInput8Ptr;

extern void* weaponSpecBodies;

void InitialiseGlobals();