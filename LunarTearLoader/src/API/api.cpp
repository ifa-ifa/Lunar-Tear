#include "api.h"
#include "Common/Logger.h"
#include "Lua/LuaCommandQueue.h"
#include "Lua/CallbackQueue.h"
#include "Game/Globals.h"
#include "Game/Functions.h"
#include "ModLoader.h"
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

        Logger::LogCategory category = API::ApiLogLevelToInternal(level);
        Logger::Log(category, ctx->name) << message;
    }


    bool API_Lua_RegisterCFunc(LT_PluginHandle handle, const char* function_name, LT_LuaCFunc pFunc) {
        if (!handle || !function_name || !pFunc) return false;
        PluginContext* ctx = static_cast<PluginContext*>(handle);

        std::lock_guard<std::mutex> lock(API::s_lua_binding_mutex);

        s_plugin_lua_bindings.push_back({ std::string(function_name), pFunc });

        Logger::Log(Verbose) << "Plugin '" << ctx->name << "' registered Lua function: " << function_name;
        return true;
    }

    void API_Lua_QueuePhaseScriptCall(LT_PluginHandle handle, const char* function_name) {
        if (!handle || !function_name) return;
        PluginContext* ctx = static_cast<PluginContext*>(handle);
        Logger::Log(Verbose, ctx->name) << "Queueing phase script call: " << function_name;
        LuaCommandQueue::QueuePhaseScriptCall(function_name);
    }

    void API_QueuePhaseUpdateTask(LT_PluginHandle handle, LT_UpdateFunc pFunc, void* userData) {
        if (!handle || !pFunc) return;
        UpdateCallbackQueue::QueueCallback(UpdateCallbackQueue::UpdateLoopType::Phase, pFunc, userData);
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


    bool API_IsModActive(LT_PluginHandle handle, const char* mod_id) {
        if (!mod_id) return false;
        return GetModPath(mod_id).has_value();
    }

    bool API_IsPluginActive(LT_PluginHandle handle, const char* plugin_name) {
        if (!plugin_name) return false;
        return API::IsPluginLoaded(plugin_name);
    }

    int API_GetModDirectory(LT_PluginHandle handle, const char* mod_id, char* out_buffer, uint32_t buffer_size) {
        if (!mod_id || !out_buffer || buffer_size == 0) return -1;

        auto mod_path_opt = GetModPath(mod_id);
        if (!mod_path_opt) {
            return -1;
        }

        strncpy_s(out_buffer, buffer_size, mod_path_opt->c_str(), _TRUNCATE);
        return strnlen_s(out_buffer, buffer_size);
    }

    void API_Lua_QueuePhaseScriptExecution(LT_PluginHandle handle, const char* script, LT_ScriptExecutionCallbackFunc callback, void* userData) {
        if (!handle || !script) return;
        PluginContext* ctx = static_cast<PluginContext*>(handle);

        Logger::Log(Verbose, ctx->name) << "Queueing non-blocking script execution.";
        LuaCommandQueue::QueuePhaseScriptExecution(script, callback, userData);
    }   


    PlayableManager* API_GetPlayableManager() {
    
        if (!playableManager) {
            return nullptr;
        }
        if (playableManager->pVtable == 0) {
            return nullptr;
        }
        return playableManager;
    
    }

    ActorPlayable* API_GetActorPlayable(PlayableManager* pm) {
        
        if (!pm) {
            return nullptr;
        }
        return pm->actorPlayable;


    }

    void* API_GetD3D11Device() {

        if (!rhiDevicePtr) {
            return nullptr;
        }

        if (!*rhiDevicePtr) {
            return nullptr;
        }

        return (*rhiDevicePtr)->d3d11Device;

    }

    void* API_GetD3D11DeviceContext() {

        if (!rhiDevicePtr) {
            return nullptr;
        }

        if (!*rhiDevicePtr) {
            return nullptr;
        }

        return (*rhiDevicePtr)->d3d11DeviceContext;

    }

    void* API_GetDXGIFactory() {

        if (!rhiDevicePtr) {
            return nullptr;
        }

        if (!*rhiDevicePtr) {
            return nullptr;
        }

        return (*rhiDevicePtr)->dxgiFactory;

    }


    void* API_GetPresentAddress() {

        void* swapChainVtable[18] = {}; 
        void* swapChain = nullptr;

        struct DummySwapChainDesc {
            void* BufferDesc;
            void* SampleDesc;
            void* BufferUsage;
            void* OutputWindow;
            BOOL Windowed;
            unsigned int BufferCount;
            unsigned int Flags;
            void* SwapEffect;
        } scd = {};
        scd.Windowed = TRUE;
        scd.BufferCount = 1;

        typedef HRESULT(__stdcall* CreateSwapChainFn)(void* factory, void* device, DummySwapChainDesc* desc, void** swapChain);
        auto CreateSwapChain = *(CreateSwapChainFn*)((char**)API_GetDXGIFactory())[10]; 

        if (CreateSwapChain(API_GetDXGIFactory(), API_GetD3D11Device(), &scd, &swapChain) != 0 || !swapChain)
            return nullptr;

        void** vtable = *(void***)swapChain;
        void* presentAddr = vtable[8]; // Present is usually at index 8

        typedef void(__stdcall* ReleaseFn)(void*);
        auto Release = *(ReleaseFn*)vtable[2];
        Release(swapChain);

        return presentAddr;
    }

    const char* API_GetVersionString() {
        static std::string version_string = LUNAR_TEAR_VERSION_STRING;
        return version_string.c_str();
    }

    uint32_t API_GetVersionMajor() {
        return LUNAR_TEAR_VERSION_MAJOR;
    }

    uint32_t API_GetVersionMinor() {
        return LUNAR_TEAR_VERSION_MINOR;
    }

    uint32_t API_GetVersionPatch() {
        return LUNAR_TEAR_VERSION_PATCH;
    }

}

namespace API {

    bool IsPluginLoaded(const std::string& pluginName) {
        std::lock_guard<std::mutex> lock(s_context_mutex);
        for (const auto& pair : s_plugin_contexts) {
            if (pair.second->name == pluginName) {
                return true;
            }
        }
        return false;
    }

    Logger::LogCategory ApiLogLevelToInternal(LT_LogLevel level) {
        switch (level) {
        case LT_LOG_INFO:    return Info;
        case LT_LOG_VERBOSE: return Verbose;
        case LT_LOG_WARNING: return Warning;
        case LT_LOG_ERROR:   return Error;
        case LT_LOG_LUA:     return Lua;
        default:             return Logger::LogCategory::Info;
        }
    }

    void Init() {

        s_gameApi.phaseBindingDispatcher = phaseBindingDispatcher;

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
        s_api.QueuePhaseUpdateTask = API_QueuePhaseUpdateTask;
        s_api.Lua_QueuePhaseScriptExecution = API_Lua_QueuePhaseScriptExecution;
        s_api.IsModActive = API_IsModActive;
        s_api.IsPluginActive = API_IsPluginActive;
        s_api.GetModDirectory = API_GetModDirectory;

        s_api.GetVersionString = API_GetVersionString;
        s_api.GetVersionMajor = API_GetVersionMajor;
        s_api.GetVersionMinor = API_GetVersionMinor;
        s_api.GetVersionPatch = API_GetVersionPatch;

        s_gameApi.processBaseAddress = g_processBaseAddress;
        s_gameApi.phaseScriptManager = phaseScriptManager;
        s_gameApi.rootScriptManager = rootScriptManager;
        s_gameApi.gameScriptManager = gameScriptManager;
        s_gameApi.playerSaveData = playerSaveData;
        s_gameApi.endingsData = endingsData;
        s_gameApi.localeData = localeData;
        s_gameApi.playerParam = playerParam;
        s_gameApi.rhiDevicePtr = rhiDevicePtr;


        s_gameApi.GetPlayableManager = API_GetPlayableManager;
        s_gameApi.GetActorPlayable = API_GetActorPlayable;



        Logger::Log(Info) << "Lunar Tear API Initialized (Version " << API_GetVersionString() << ")";
    }

    const LunarTearAPI* GetAPI() {
        return &s_api;
    }

    void InitializePlugin(const std::string& mod_id, HMODULE pluginModule) {
        PluginContext* context_ptr = nullptr;
        {
            auto context = std::make_unique<PluginContext>();
            context->name = mod_id;
            context->moduleHandle = pluginModule;

            auto mod_path_opt = GetModPath(mod_id);
            if (mod_path_opt) {
                std::filesystem::path config_path = *mod_path_opt;
                config_path /= "config.ini";

                context->configReader = std::make_unique<INIReader>(config_path.string());
                if (context->configReader->ParseError() != 0) {
                    Logger::Log(Verbose) << "Could not parse config for plugin '" << mod_id << "': " << config_path.string();
                }
            }
            else {
                Logger::Log(Warning) << "Could not find path for mod '" << mod_id << "' to load its config.";
            }


            context_ptr = context.get(); 

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