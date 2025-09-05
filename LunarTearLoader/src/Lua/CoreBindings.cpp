#include "Game/Functions.h"
#include "Common/Logger.h"
#include "Lua/CoreBindings.h"
#include "Lua/LuaCommandQueue.h"
#include "API/Api.h"
#include "ModLoader.h"
#include <INIReader.h>

using enum Logger::LogCategory;

void _LTLog(ScriptState* scriptState) {
    const char* modName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    int logLevel = GetArgumentInt(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* logString = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));



    Logger::Log(API::ApiLogLevelToInternal((LT_LogLevel)logLevel), modName) << logString;
}

void Binding_LTLog(void* L) {
    luaBindingDispatcher(L, &_LTLog);
}

void _LTIsModActive(ScriptState* scriptState) {
    const char* mod_id = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));

    bool isActive = false;
    if (mod_id) {
        isActive = GetModPath(mod_id).has_value();
    }

    SetArgumentInt(scriptState->returnBuffer, isActive ? 1 : 0);
    scriptState->returnArgCount = 1;
}

void Binding_LTIsModActive(void* L) {
    luaBindingDispatcher(L, &_LTIsModActive);
}

void _LTIsPluginActive(ScriptState* scriptState) {
    const char* pluginName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));

    bool isActive = false;
    if (pluginName) {
        isActive = API::IsPluginLoaded(pluginName);
    }

    SetArgumentInt(scriptState->returnBuffer, isActive ? 1 : 0);
    scriptState->returnArgCount = 1;
}

void Binding_LTIsPluginActive(void* L) {
    luaBindingDispatcher(L, &_LTIsPluginActive);
}




void _LTConfigGetInt(ScriptState* scriptState) {
    const char* mod_id = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    int   defaultValue = GetArgumentInt(GetArgumentPointer(scriptState->argBuffer, 3));

    auto fail = [&](int value) {
        SetArgumentInt(scriptState->returnBuffer, value);
        scriptState->returnArgCount = 1;
        };

    if (!mod_id || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    auto mod_path_opt = GetModPath(mod_id);
    if (!mod_path_opt) {
        Logger::Log(Error) << "Failed to find mod with ID '" << mod_id << "' for config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;

        fail(defaultValue);
        return;
    }

    std::filesystem::path config_path = *mod_path_opt;
    config_path /= "config.ini";
    INIReader reader(config_path.string());

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << config_path.string()
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    int result = reader.GetInteger(sectionName, keyName, defaultValue);

    SetArgumentInt(scriptState->returnBuffer, result);
    scriptState->returnArgCount = 1;
}


void Binding_LTConfigGetInt(void* L) {
    luaBindingDispatcher(L, &_LTConfigGetInt);
}



void _LTConfigGetReal(ScriptState* scriptState) {
    const char* mod_id = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    float defaultValue = GetArgumentFloat(GetArgumentPointer(scriptState->argBuffer, 3));

    auto fail = [&](float value) {
        SetArgumentFloat(scriptState->returnBuffer, value);
        scriptState->returnArgCount = 1;
        };

    if (!mod_id || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    auto mod_path_opt = GetModPath(mod_id);
    if (!mod_path_opt) {
        Logger::Log(Error) << "Failed to find mod with ID '" << mod_id << "' for config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;

        fail(defaultValue);
        return;
    }

    std::filesystem::path config_path = *mod_path_opt;
    config_path /= "config.ini";
    INIReader reader(config_path.string());

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << config_path.string()
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    float result = static_cast<float>(reader.GetReal(sectionName, keyName, defaultValue));

    fail(result);
}


void Binding_LTConfigGetReal(void* L) {
    luaBindingDispatcher(L, &_LTConfigGetReal);
}




void _LTConfigGetBool(ScriptState* scriptState) {
    const char* mod_id = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    bool defaultValue = GetArgumentInt(GetArgumentPointer(scriptState->argBuffer, 3)) != 0;

    auto fail = [&](bool value) {
        SetArgumentInt(scriptState->returnBuffer, value ? 1 : 0);
        scriptState->returnArgCount = 1;
        };

    if (!mod_id || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    auto mod_path_opt = GetModPath(mod_id);
    if (!mod_path_opt) {
        Logger::Log(Error) << "Failed to find mod with ID '" << mod_id << "' for config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;

        fail(defaultValue);
        return;
    }

    std::filesystem::path config_path = *mod_path_opt;
    config_path /= "config.ini";
    INIReader reader(config_path.string());

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << config_path.string()
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    bool result = reader.GetBoolean(sectionName, keyName, defaultValue);


    fail(result);
}





void Binding_LTConfigGetBool(void* L) {
    luaBindingDispatcher(L, &_LTConfigGetBool);
}

void _LTConfigGetString(ScriptState* scriptState) {
    const char* mod_id = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    const char* defaultValue = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 3));

    auto fail = [&](const char* value) {
        SetArgumentString(scriptState->returnBuffer, value);
        scriptState->returnArgCount = 1;
        };

    if (!mod_id || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << (defaultValue ? defaultValue : "<null>");
        fail(defaultValue);
        return;
    }

    auto mod_path_opt = GetModPath(mod_id);
    if (!mod_path_opt) {
        Logger::Log(Error) << "Failed to find mod with ID '" << mod_id << "' for config lookup: "
            << (mod_id ? mod_id : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;

        fail(defaultValue);
        return;
    }

    std::filesystem::path config_path = *mod_path_opt;
    config_path /= "config.ini";
    INIReader reader(config_path.string());

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << config_path.string()
            << " - Returning default value: " << (defaultValue ? defaultValue : "<null>");
        fail(defaultValue);
        return;
    }

    std::string result = reader.Get(sectionName, keyName, defaultValue ? defaultValue : "");

    fail(result.c_str());
}


void Binding_LTConfigGetString(void* L) {
    luaBindingDispatcher(L, &_LTConfigGetString);
}

void _LTGetModDirectory(ScriptState* scriptState) {
    const char* mod_id = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));

    std::string path_str = "";
    if (mod_id) {
        auto mod_path_opt = GetModPath(mod_id);
        if (mod_path_opt) {
            path_str = *mod_path_opt;
        }
    }

    SetArgumentString(scriptState->returnBuffer, path_str.c_str());
    scriptState->returnArgCount = 1;
}

void Binding_LTGetModDirectory(void* L) {
    luaBindingDispatcher(L, &_LTGetModDirectory);
}


void _ReportResult(ScriptState* state) {
    const char* serializedResult = GetArgumentString(GetArgumentPointer(state->argBuffer, 0));
    LuaCommandQueue::ReportResult(serializedResult);
}

void Binding_ReportResult(void* L) {
    luaBindingDispatcher(L, &_ReportResult);
}


void _GetScript(ScriptState* state) {
    std::string script;
    if (LuaCommandQueue::GetNextScript(script)) {
        SetArgumentString(state->returnBuffer, script.c_str());
    }
    else {
        SetArgumentString(state->returnBuffer, ""); // Return empty string if no command
    }
    state->returnArgCount = 1;
}


void Binding_GetScript(void* L) {
    luaBindingDispatcher(L, &_GetScript);
}



LuaCBinding* GetCoreBindings() {

    static LuaCBinding coreBindings[] = {
    { "_LTLog",             Binding_LTLog },
    { "_LTConfigGetString", Binding_LTConfigGetString },
    { "_LTConfigGetBool",   Binding_LTConfigGetBool },
    { "_LTConfigGetReal",   Binding_LTConfigGetReal },
    { "_LTConfigGetInt",    Binding_LTConfigGetInt },
    { "_LTIsModActive",     Binding_LTIsModActive },
    { "_LTIsPluginActive",  Binding_LTIsPluginActive },
    { "_LTGetModDirectory", Binding_LTGetModDirectory },

    { "_LTLua_type", luaB_type },
    { "_LTLua_xpcall", luaB_xpcall },
    { "_LTLua_tostring", luaB_tostring },
    { "_LTLua_loadstring", luaB_loadstring },
    { "_LTLua_table_getn", luaB_table_getn },

    // Internal use only
    { "_ifaifa_LTCore_GetScript", Binding_GetScript }, 
    { "_ifaifa_LTCore_ReportResult", Binding_ReportResult },


    { NULL, NULL }
    };


    return coreBindings;
}