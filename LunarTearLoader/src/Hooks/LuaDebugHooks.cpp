#include "Hooks.h"
#include "Common/Logger.h"
#include "Game/Functions.h"
#include "Lua/LuaCommandQueue.h"
#include <MinHook.h>

using enum Logger::LogCategory;

namespace {
    typedef void(__fastcall* Binding_DebugPrint_t)(void* param_1);
    Binding_DebugPrint_t Binding_DebugPrintOriginal = nullptr;
}

void c_DebugPrint(ScriptState* scriptState) { // Not to be confused with lua_state

    void* arg = GetArgumentPointer(scriptState->argBuffer, 0);
    char* str = (char*)GetArgumentString(arg);
    Logger::Log(Lua) << std::string(str);
}

void Binding_DebugPrintDetoured(void* lua_state) {

    luaBindingDispatcher(lua_state, &c_DebugPrint);
    return;
}


bool InstallDebugHooks() {

    // This target is the c binding for the games internal _DebugPrint function, we hook it to feed our own logging system
    void* Binding_DebugPrintTarget = (void*)(g_processBaseAddress + 0x6df8f0);

    if (MH_CreateHookEx(Binding_DebugPrintTarget, &Binding_DebugPrintDetoured, &Binding_DebugPrintOriginal) != MH_OK) {
        Logger::Log(Error) << "Could not create debug hook";
        return false;
    }
    return true;
}