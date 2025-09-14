#pragma once

#include <cstdint>
#include "Game/Types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUNAR_TEAR_VERSION_MAJOR 1
#define LUNAR_TEAR_VERSION_MINOR 2
#define LUNAR_TEAR_VERSION_PATCH 1

#define LUNAR_TEAR_API_VERSION ((LUNAR_TEAR_VERSION_MAJOR << 16) | \
                                (LUNAR_TEAR_VERSION_MINOR << 8)  | \
                                LUNAR_TEAR_VERSION_PATCH)

#define LT_VERSION_STR_HELPER(x) #x
#define LT_VERSION_STR(x) LT_VERSION_STR_HELPER(x)
#define LUNAR_TEAR_VERSION_STRING LT_VERSION_STR(LUNAR_TEAR_VERSION_MAJOR) "." \
                                  LT_VERSION_STR(LUNAR_TEAR_VERSION_MINOR) "." \
                                  LT_VERSION_STR(LUNAR_TEAR_VERSION_PATCH)



    typedef void* LT_PluginHandle;

    typedef enum {
        LT_LOG_INFO,
        LT_LOG_VERBOSE,
        LT_LOG_WARNING,
        LT_LOG_ERROR,
        LT_LOG_LUA, 
    } LT_LogLevel;

    typedef enum {
        LT_HOOK_OK = 0,
        LT_HOOK_ERROR_ALREADY_CREATED,
        LT_HOOK_ERROR_NOT_CREATED,
        LT_HOOK_ERROR_ENABLED,
        LT_HOOK_ERROR_DISABLED,
        LT_HOOK_ERROR_NOT_EXECUTABLE,
        LT_HOOK_ERROR_UNSUPPORTED_FUNCTION,
        LT_HOOK_ERROR_MEMORY_ALLOC,
        LT_HOOK_ERROR_MEMORY_PROTECT,
        LT_HOOK_ERROR_MODULE_NOT_FOUND,
        LT_HOOK_ERROR_FUNCTION_NOT_FOUND,
        LT_HOOK_UNKNOWN = -1
    } LT_HookStatus;

    typedef enum {
        LT_LUA_RESULT_NIL,
        LT_LUA_RESULT_STRING,
        LT_LUA_RESULT_NUMBER,
        LT_LUA_RESULT_ERROR_SYNTAX,
        LT_LUA_RESULT_ERROR_RUNTIME,
        LT_LUA_RESULT_ERROR_UNSUPPORTED_TYPE
    } LT_LuaResultType;

    typedef struct {
        LT_LuaResultType type;
        union {
            const char* stringValue; // Valid if type is STRING or any ERROR
            double      numberValue;   
        } value;
    } LT_LuaResult;

    typedef void (*LT_UpdateFunc)(void* userData);
    typedef void (*LT_LuaCFunc)(void* LuaState);
    typedef void (*LT_ScriptExecutionCallbackFunc)(const LT_LuaResult* result, void* userData);

    typedef struct LT_GameAPI {

        uintptr_t processBaseAddress;

        int (*phaseBindingDispatcher)(void* luaState, void* CFunc);

        void* (*GetArgumentPointer)(void* argBuffer, int index);
        const char* (*GetArgumentString)(void* arg);
        int (*GetArgumentInt)(void* arg);
        float (*GetArgumentFloat)(void* arg);

        void* (*SetArgumentFloat)(void* returnBuffer, float value);
        void* (*SetArgumentInt)(void* returnBuffer, int value);
        void* (*SetArgumentString)(void* returnBuffer, const char* value);

        bool (*AnyEndingSeen)(EndingsData* endingsData);
        bool (*EndingESeen)(EndingsData* endingsData);
        bool (*CheckRouteEActive)(EndingsData* endingsData);
        char* (*GetLocalizedString)(void* localedata, int strId);

        int (*getCurrentLevel)(void);
        uint64_t(*getPlayerItemCount)(PlayerSaveData* saveData, unsigned int itemId);
        void (*setCharacterLevelInSaveData)(PlayerSaveData* saveData, unsigned int charIndex, int newLevel);
        void (*setPlayerItemCount)(PlayerSaveData* saveData, unsigned int itemId, char count);

        void (*SpawnEnemyGroup)(int EnemyGroupID, unsigned char param_2, float param_3);

        void (*GameFlagOn)(void* unused, unsigned int flagId);
        void (*GameFlagOff)(void* unused, unsigned int flagId);
        uint64_t(*IsGameFlag)(void* unused, unsigned int flagId);
        void (*SnowGameFlagOn)(void* unused, unsigned int flagId);
        void (*SnowGameFlagOff)(void* unused, unsigned int flagId);
        uint64_t(*IsSnowGameFlag)(void* unused, unsigned int flagId);

        PhaseScriptManager* phaseScriptManager;
        RootScriptManager* rootScriptManager;
        GameScriptManager* gameScriptManager;
        PlayerSaveData* playerSaveData;
        EndingsData* endingsData;
        void* localeData;

        PlayableManager* (*GetPlayableManager)(void);
        ActorPlayable* (*GetActorPlayable)(PlayableManager* pm);

        CPlayerParam* playerParam;


    } LT_GameAPI;


    typedef struct LunarTearAPI {

        uint32_t api_version; 
        LT_GameAPI* game;

        void (*Log)(LT_PluginHandle handle, LT_LogLevel level, const char* message);
        int(*Config_GetString)(LT_PluginHandle handle, const char* section, const char* key, const char* default_value, char* out_buffer, uint32_t buffer_size);
        long (*Config_GetInteger)(LT_PluginHandle handle, const char* section, const char* key, long default_value);
        double (*Config_GetReal)(LT_PluginHandle handle, const char* section, const char* key, double default_value);
        bool (*Config_GetBoolean)(LT_PluginHandle handle, const char* section, const char* key, bool default_value);
        LT_HookStatus(*Hook_Create)(LT_PluginHandle handle, void* pTarget, void* pDetour, void** ppOriginal);
        LT_HookStatus(*Hook_Enable)(LT_PluginHandle handle, void* pTarget);
        bool (*Lua_RegisterCFunc)(LT_PluginHandle handle, const char* function_name, LT_LuaCFunc pFunc);
        void (*Lua_QueuePhaseScriptCall)(LT_PluginHandle handle, const char* function_name); // Not reccomended anymore - use Lua_QueuePhaseScriptExecution
        bool (*IsModActive)(LT_PluginHandle handle, const char* mod_name);
        bool (*IsPluginActive)(LT_PluginHandle handle, const char* plugin_name);
        int (*
            
            
            Directory)(LT_PluginHandle handle, const char* mod_name, char* out_buffer, uint32_t buffer_size);

        void (*QueuePhaseUpdateTask)(LT_PluginHandle handle, LT_UpdateFunc pFunc, void* userData);
        void (*Lua_QueuePhaseScriptExecution)(LT_PluginHandle handle, const char* script, LT_ScriptExecutionCallbackFunc callback, void* userData);

        const char* (*GetVersionString)(void);
        uint32_t(*GetVersionMajor)(void);
        uint32_t(*GetVersionMinor)(void);
        uint32_t(*GetVersionPatch)(void);


    } LunarTearAPI;

    typedef void (*LunarTearPluginInitFunc)(const LunarTearAPI* api, LT_PluginHandle handle);


#ifdef __cplusplus
}
#endif

