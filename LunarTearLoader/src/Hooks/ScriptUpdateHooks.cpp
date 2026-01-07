#include "Hooks.h"

#include "ModLoader.h"
#include "Common/Settings.h"
#include "Common/Logger.h"
#include "Game/Functions.h"
#include "Common/Dump.h"
#include "Lua/LuaCommandQueue.h"
#include <MinHook.h>
#include "Lua/CallbackQueue.h"
#include <filesystem>
#include<vector>

using enum Logger::LogCategory;

namespace {

    // This is a function called many times per second on the scripting thread, good hook target to execute lua commands from our own queue
    typedef void(__fastcall* UpdatePhaseScriptManager_t)(void* phsm);
    UpdatePhaseScriptManager_t UpdatePhaseScriptManagerOriginal = nullptr;

}

void UpdatePhaseScriptManagerDetoured(PhaseScriptManager* phsm) {

    LuaCommandQueue::ProcessQueue(&(phsm->scriptManager));
    UpdateCallbackQueue::ProcessCallbacks(UpdateCallbackQueue::UpdateLoopType::Phase);

    return UpdatePhaseScriptManagerOriginal(phsm);
}


bool InstallScriptUpdateHooks() {

    void* UpdatePhaseScriptManagerTarget = (void*)(g_processBaseAddress + 0x415710);

    InstallHook(UpdatePhaseScriptManagerTarget, &UpdatePhaseScriptManagerDetoured, &UpdatePhaseScriptManagerOriginal, "Update phsm");

    return true;
}