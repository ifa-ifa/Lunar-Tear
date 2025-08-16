#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <mutex>
#include <utility> 
#include "Game/Types.h"
#include "API/LunarTear.h"

namespace API {
    void Init();
    const LunarTearAPI* GetAPI();
    void InitializePlugin(const std::string& pluginName, HMODULE pluginModule);
    bool IsPluginLoaded(const std::string& pluginName);


    extern std::mutex s_lua_binding_mutex;

    // Get all registered Lua bindings from plugins
    const std::vector<std::pair<std::string, LT_LuaCFunc>>& GetPluginLuaBindings();
}