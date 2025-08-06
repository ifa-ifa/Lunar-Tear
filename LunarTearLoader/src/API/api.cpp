#include "api.h"
#include "Common/Logger.h"
#include "Lua/LuaCommandQueue.h"
#include "Game/Globals.h"
#include "Game/Functions.h"
#include <MinHook.h>
#include <map>
#include <mutex>
#include <INIReader.h>
#include <vector>
#include <string>
#include "API/LunarTear.h"

using enum Logger::LogCategory;

namespace {

    struct PluginContext {
        std::string name;
        std::unique_ptr<INIReader> configReader;
        HMODULE moduleHandle;
    };

    static LunarTearAPI s_api;
    static LT_GameAPI s_gameApi;

    static std::map<HMODULE, std::unique_ptr<PluginContext>> s_plugin_contexts;
    static std::mutex s_context_mutex;


    static std::vector<std::pair<std::string, LT_LuaCFunc>> s_plugin_lua_bindings;


    void API_Log(LT_PluginHandle handle, LT_LogLevel level, const char* message) {
        if (!handle) return;
        PluginContext* ctx = static_cast<PluginContext*>(handle);

        Logger::LogCategory category = Info;
        switch (level) {
        case LT_LOG_INFO:    category = Info;    break;
        case LT_LOG_VERBOSE: category = Verbose; break;
        case LT_LOG_WARNING: category = Warning; break;
        case LT_LOG_ERROR:   category = Error;   break;
        case LT_LOG_LUA:     category = Lua;     break;
        }
        Logger::Log(category, ctx->name) << message;
    }


    bool API_Lua_RegisterCFunc(LT_PluginHandle handle, const char* function_name, LT_LuaCFunc pFunc) {
        if (!handle || !function_name || !pFunc) return false;
        PluginContext* ctx = static_cast<PluginContext*>(handle);

        std::lock_guard<std::mutex> lock(API::s_lua_binding_mutex);

        s_plugin_lua_bindings.push_back({ std::string(function_name), pFunc });

        Logger::Log(Info) << "Plugin '" << ctx->name << "' registered Lua function: " << function_name;
        return true;
    }

    void API_Lua_QueuePhaseScriptCall(LT_PluginHandle handle, const char* function_name) {
        if (!handle || !function_name) return;
        PluginContext* ctx = static_cast<PluginContext*>(handle);
        Logger::Log(Verbose, ctx->name) << "Queueing phase script call: " << function_name;
        LuaCommandQueue::QueuePhaseScriptCall(function_name);
    }

    int API_Config_GetString(LT_PluginHandle handle, const char* section, const char* key, const char* default_value, char* out_buffer, uint32_t buffer_size) {
        if (!handle || !out_buffer || buffer_size == 0) return -1;
        PluginContext* ctx = static_cast<PluginContext*>(handle);

        if (!ctx->configReader) {
            strncpy_s(out_buffer, buffer_size, default_value, _TRUNCATE);
            return (int)strlen(default_value);
        }

        std::string value = ctx->configReader->Get(section, key, default_value);
        strncpy_s(out_buffer, buffer_size, value.c_str(), _TRUNCATE);

        return strnlen_s(out_buffer, buffer_size);
    }

    long API_Config_GetInteger(LT_PluginHandle handle, const char* section, const char* key, long default_value) {
        if (!handle) return default_value;
        PluginContext* ctx = static_cast<PluginContext*>(handle);
        if (!ctx->configReader) return default_value;
        return ctx->configReader->GetInteger(section, key, default_value);
    }

    double API_Config_GetReal(LT_PluginHandle handle, const char* section, const char* key, double default_value) {
        if (!handle) return default_value;
        PluginContext* ctx = static_cast<PluginContext*>(handle);
        if (!ctx->configReader) return default_value;
        return ctx->configReader->GetReal(section, key, default_value);
    }

    bool API_Config_GetBoolean(LT_PluginHandle handle, const char* section, const char* key, bool default_value) {
        if (!handle) return default_value;
        PluginContext* ctx = static_cast<PluginContext*>(handle);
        if (!ctx->configReader) return default_value;
        return ctx->configReader->GetBoolean(section, key, default_value);
    }

    LT_HookStatus ToLTHookStatus(MH_STATUS status) {
        switch (status) {
        case MH_OK:                         return LT_HOOK_OK;
        case MH_ERROR_ALREADY_CREATED:      return LT_HOOK_ERROR_ALREADY_CREATED;
        case MH_ERROR_NOT_CREATED:          return LT_HOOK_ERROR_NOT_CREATED;
        case MH_ERROR_ENABLED:              return LT_HOOK_ERROR_ENABLED;
        case MH_ERROR_DISABLED:             return LT_HOOK_ERROR_DISABLED;
        case MH_ERROR_NOT_EXECUTABLE:       return LT_HOOK_ERROR_NOT_EXECUTABLE;
        case MH_ERROR_UNSUPPORTED_FUNCTION: return LT_HOOK_ERROR_UNSUPPORTED_FUNCTION;
        case MH_ERROR_MEMORY_ALLOC:         return LT_HOOK_ERROR_MEMORY_ALLOC;
        case MH_ERROR_MEMORY_PROTECT:       return LT_HOOK_ERROR_MEMORY_PROTECT;
        case MH_ERROR_MODULE_NOT_FOUND:     return LT_HOOK_ERROR_MODULE_NOT_FOUND;
        case MH_ERROR_FUNCTION_NOT_FOUND:   return LT_HOOK_ERROR_FUNCTION_NOT_FOUND;
        default:                            return LT_HOOK_UNKNOWN;
        }
    }

    LT_HookStatus API_Hook_Create(LT_PluginHandle handle, void* pTarget, void* pDetour, void** ppOriginal) {
        return ToLTHookStatus(MH_CreateHook(pTarget, pDetour, ppOriginal));
    }

    LT_HookStatus API_Hook_Enable(LT_PluginHandle handle, void* pTarget) {
        return ToLTHookStatus(MH_EnableHook(pTarget));
    }
}

namespace API {

    void Init() {
        s_gameApi.GetArgumentPointer = GetArgumentPointer;
        s_gameApi.GetArgumentString = GetArgumentString;
        s_gameApi.GetArgumentInt = GetArgumentInt;
        s_gameApi.GetArgumentFloat = GetArgumentFloat;
        s_gameApi.SetArgumentFloat = SetArgumentFloat;
        s_gameApi.SetArgumentInt = SetArgumentInt;
        s_gameApi.SetArgumentString = SetArgumentString; 

        s_gameApi.AnyEndingSeen = (bool (*)(EndingsData*))AnyEndingSeen;
        s_gameApi.EndingESeen = (bool (*)(EndingsData*))EndingESeen;
        s_gameApi.CheckRouteEActive = (bool (*)(EndingsData*))CheckRouteEActive;
        s_gameApi.GetLocalizedString = (char* (*)(void*, int))GetLocalizedString;

        s_gameApi.getCurrentLevel = getCurrentLevel;
        s_gameApi.getPlayerItemCount = (uint64_t(*)(PlayerSaveData*, unsigned int))getPlayerItemCount;
        s_gameApi.setCharacterLevelInSaveData = (void (*)(PlayerSaveData*, unsigned int, int))setCharacterLevelInSaveData;
        s_gameApi.setPlayerItemCount = (void (*)(PlayerSaveData*, unsigned int, char))setPlayerItemCount;
        s_gameApi.SpawnEnemyGroup = SpawnEnemyGroup;
        s_gameApi.GameFlagOn = GameFlagOn;
        s_gameApi.GameFlagOff = GameFlagOff;
        s_gameApi.IsGameFlag = IsGameFlag;
        s_gameApi.SnowGameFlagOn = SnowGameFlagOn;
        s_gameApi.SnowGameFlagOff = SnowGameFlagOff;
        s_gameApi.IsSnowGameFlag = IsSnowGameFlag;

        s_api.api_version = LUNAR_TEAR_API_VERSION;
        s_api.game = &s_gameApi;

        s_api.Config_GetString = API_Config_GetString;
        s_api.Config_GetInteger = API_Config_GetInteger;
        s_api.Config_GetReal = API_Config_GetReal;
        s_api.Config_GetBoolean = API_Config_GetBoolean;

        s_api.Log = API_Log;
        s_api.Hook_Create = API_Hook_Create;
        s_api.Hook_Enable = API_Hook_Enable;
        s_api.Lua_RegisterCFunc = API_Lua_RegisterCFunc;
        s_api.Lua_QueuePhaseScriptCall = API_Lua_QueuePhaseScriptCall;

        s_api.processBaseAddress = g_processBaseAddress;
        s_api.phaseScriptManager = (PhaseScriptManager*)phaseScriptManager;
        s_api.rootScriptManager = (RootScriptManager*)rootScriptManager;
        s_api.gameScriptManager = (GameScriptManager*)gameScriptManager;
        s_api.playerSaveData = (PlayerSaveData*)playerSaveData;
        s_api.endingsData = (EndingsData*)endingsData;
        s_api.localeData = localeData;

        Logger::Log(Info) << "Lunar Tear API Initialized (Version " << s_api.api_version << ")";
    }

    const LunarTearAPI* GetAPI() {
        return &s_api;
    }

    void InitializePlugin(const std::string& pluginName, HMODULE pluginModule) {
        PluginContext* context_ptr = nullptr;
        {
            auto context = std::make_unique<PluginContext>();
            context->name = pluginName;
            context->moduleHandle = pluginModule;

            std::filesystem::path config_path = "LunarTear/mods/";
            config_path /= pluginName;
            config_path /= pluginName + ".ini";

            context->configReader = std::make_unique<INIReader>(config_path.string());
            if (context->configReader->ParseError() != 0) {
                Logger::Log(Warning) << "Could not parse config for plugin '" << pluginName << "': " << config_path.string();
            }

            context_ptr = context.get(); // Get raw pointer before moving ownership

            std::lock_guard<std::mutex> lock(s_context_mutex);
            s_plugin_contexts[pluginModule] = std::move(context);
        }

        LunarTearPluginInitFunc init_func = (LunarTearPluginInitFunc)GetProcAddress(pluginModule, "LunarTearPluginInit");
        if (init_func) {
            init_func(GetAPI(), static_cast<LT_PluginHandle>(context_ptr));
        }
    }

    std::mutex s_lua_binding_mutex;

    const std::vector<std::pair<std::string, LT_LuaCFunc>>& API::GetPluginLuaBindings() {
        return s_plugin_lua_bindings;
    }
}
