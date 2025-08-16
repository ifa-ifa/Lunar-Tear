#include "Game/Functions.h"
#include "Common/Logger.h"
#include "Lua/CoreBindings.h"
#include "API/Api.h"
#include <INIReader.h>

using enum Logger::LogCategory;

void _LTLog(ScriptState* scriptState) { 
    const char* modName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    int logLevel = GetArgumentInt(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* logString = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));



    Logger::Log((Logger::LogCategory)logLevel, modName) << logString;
}

void Binding_LTLog(void* L) {
    luaBindingDispatcher(L, &_LTLog);
}

void _LTIsModActive(ScriptState* scriptState) {
    const char* modName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));

    bool isActive = false;
    if (modName) {
        std::filesystem::path mod_path = "LunarTear/mods/";
        mod_path /= modName;
        isActive = std::filesystem::exists(mod_path) && std::filesystem::is_directory(mod_path);
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
    const char* modName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    int   defaultValue = GetArgumentInt(GetArgumentPointer(scriptState->argBuffer, 3));

    auto fail = [&](int value) {
        SetArgumentInt(scriptState->returnBuffer, value);
        scriptState->returnArgCount = 1;
        };

    if (!modName || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (modName ? modName : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    std::string filePath = std::string("LunarTear/mods/") + modName + "/" + modName + ".ini";
    INIReader reader(filePath);

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << filePath
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
    const char* modName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    float defaultValue = GetArgumentFloat(GetArgumentPointer(scriptState->argBuffer, 3));

    auto fail = [&](float value) {
        SetArgumentFloat(scriptState->returnBuffer, value);
        scriptState->returnArgCount = 1;
        };

    if (!modName || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (modName ? modName : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    std::string filePath = std::string("LunarTear/mods/") + modName + "/" + modName + ".ini";
    INIReader reader(filePath);

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << filePath
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
    const char* modName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    bool defaultValue = GetArgumentInt(GetArgumentPointer(scriptState->argBuffer, 3)) != 0;

    auto fail = [&](bool value) {
        SetArgumentInt(scriptState->returnBuffer, value ? 1 : 0);
        scriptState->returnArgCount = 1;
        };

    if (!modName || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (modName ? modName : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << defaultValue;
        fail(defaultValue);
        return;
    }

    std::string filePath = std::string("LunarTear/mods/") + modName + "/" + modName + ".ini";
    INIReader reader(filePath);

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << filePath
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
    const char* modName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 0));
    const char* sectionName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 1));
    const char* keyName = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 2));
    const char* defaultValue = GetArgumentString(GetArgumentPointer(scriptState->argBuffer, 3));

    auto fail = [&](const char* value) {
        SetArgumentString(scriptState->returnBuffer, value);
        scriptState->returnArgCount = 1;
        };

    if (!modName || !sectionName || !keyName) {
        Logger::Log(Error)
            << "Invalid config lookup: "
            << (modName ? modName : "<null>") << ", "
            << (sectionName ? sectionName : "<null>") << ", "
            << (keyName ? keyName : "<null>")
            << " - Returning default value: " << (defaultValue ? defaultValue : "<null>");
        fail(defaultValue);
        return;
    }

    std::string filePath = std::string("LunarTear/mods/") + modName + "/" + modName + ".ini";
    INIReader reader(filePath);

    if (reader.ParseError() != 0) {
        Logger::Log(Error)
            << "Failed to load config file: " << filePath
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



LuaCBinding coreBindings[] = {
    { "_LTLog",             Binding_LTLog },
    { "_LTConfigGetString", Binding_LTConfigGetString },
    { "_LTConfigGetBool",   Binding_LTConfigGetBool },
    { "_LTConfigGetReal",   Binding_LTConfigGetReal },
    { "_LTConfigGetInt",    Binding_LTConfigGetInt },
    { "_LTIsModActive",     Binding_LTIsModActive },
    { "_LTIsPluginActive",  Binding_LTIsPluginActive },
    
    { NULL, NULL }
};

LuaCBinding* GetCoreBindings() {
    return coreBindings;
}