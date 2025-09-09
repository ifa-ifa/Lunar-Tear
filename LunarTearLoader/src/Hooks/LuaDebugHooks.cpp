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


std::string ShiftJISToUTF8(const char* shift_jis_str) {
    if (!shift_jis_str || shift_jis_str[0] == '\0') {
        return "";
    }

    int wide_char_len = MultiByteToWideChar(932, 0, shift_jis_str, -1, NULL, 0);
    if (wide_char_len == 0) {
        return "[Encoding Error: SJIS -> UTF-16 failed]";
    }
    std::wstring wide_str(wide_char_len, 0);
    MultiByteToWideChar(932, 0, shift_jis_str, -1, &wide_str[0], wide_char_len);

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8_len == 0) {
        return "[Encoding Error: UTF-16 -> UTF-8 failed]";
    }
    std::string utf8_str(utf8_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_len, NULL, NULL);

    utf8_str.resize(utf8_len - 1);

    return utf8_str;
}


void c_DebugPrint(ScriptState* scriptState) { // Not to be confused with lua_state

    void* arg = GetArgumentPointer(scriptState->argBuffer, 0);
    char* str_sjis = (char*)GetArgumentString(arg);
    Logger::Log(Lua) << ShiftJISToUTF8(str_sjis);
}

void Binding_DebugPrintDetoured(void* lua_state) {

    phaseBindingDispatcher(lua_state, &c_DebugPrint);
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