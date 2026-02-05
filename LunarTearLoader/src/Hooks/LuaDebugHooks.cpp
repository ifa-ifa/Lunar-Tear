#include "Hooks.h"
#include "Common/Logger.h"
#include "Game/Functions.h"
#include "Lua/LuaCommandQueue.h"
#include <MinHook.h>

using enum Logger::LogCategory;

namespace {
    typedef void(__fastcall* Binding_DebugPrint_t)(void* param_1);
    Binding_DebugPrint_t Binding_DebugPrintOriginal = nullptr;

    typedef int(__fastcall* lua_pcall_t)(void*, int, int, int);
    lua_pcall_t lua_pcall_original = nullptr;

    typedef int(__fastcall* lua_resume_t)(void*, int);
    lua_resume_t lua_resume_original = nullptr;

	typedef int(__fastcall* luaL_loadbuffer_t)(void*, char*, size_t, char*);
    luaL_loadbuffer_t luaL_loadbuffer_original = nullptr;
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


int lua_pcall_detoured(void* lua_state, int nargs, int nresults, int errfunc) {

    int res = lua_pcall_original(lua_state, nargs, nresults, errfunc);
    if (res != 0) {
		const char* errorMsg = lua_tostring(lua_state, -1);
        Logger::Log(Error) << "lua_pcall returned status " << res << ":  " << ShiftJISToUTF8(errorMsg);
	}
    return res;

}


int lua_resume_detoured(void* lua_state, int narg) {

    int res = lua_resume_original(lua_state, narg);
    if (res != 0) {
        const char* errorMsg = lua_tostring(lua_state, -1);
        Logger::Log(Error) << "lua_resume returned status " << res << ":  " << ShiftJISToUTF8(errorMsg);
    }
	return res;

}

int luaL_loadbuffer_detoured(void* lua_state, char* buff, size_t size, char* name) {
    int res = luaL_loadbuffer_original(lua_state, buff, size, name);
    if (res != 0) {
        const char* errorMsg = lua_tostring(lua_state, -1);
        Logger::Log(Error) << "luaL_loadbuffer returned status " << res << ":  " << ShiftJISToUTF8(errorMsg);
    }
    return res;
}

bool InstallDebugHooks() {

    // This target is the c binding for the games internal _DebugPrint function, we hook it to feed our own logging system
    void* Binding_DebugPrintTarget = (void*)(g_processBaseAddress + 0x6df8f0);

	void* lua_pcall_target = (void*)(g_processBaseAddress + 0x3d6c00);
	void* lua_resume_target = (void*)(g_processBaseAddress + 0x3de4b0);
	void* luaL_loadbuffer_target = (void*)(g_processBaseAddress + 0x3d8290);

    InstallHook(Binding_DebugPrintTarget, &Binding_DebugPrintDetoured, &Binding_DebugPrintOriginal, "DebugPrint");
	InstallHook(lua_pcall_target, &lua_pcall_detoured, &lua_pcall_original, "lua_pcall");
	InstallHook(lua_resume_target, &lua_resume_detoured, &lua_resume_original, "lua_resume");
	InstallHook(luaL_loadbuffer_target, &luaL_loadbuffer_detoured, &luaL_loadbuffer_original, "luaL_loadbuffer");

    return true;
}