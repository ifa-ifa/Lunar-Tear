Any dll file found in the root directory of a mod will be injected, provided plugins are enabled in LunarTear.ini. The loader provides an api that exposes game functions, scripting integration, logging, hooking, and config functionality. Your plugin does not have to use the API, and even if it does, you do not have to use all the functionality (the hooking and config interface is very basic, for advanced use you can use your own libraries). If you do want to use the apis built in config, place a config file named config.ini in your mod's root directory (mod name is name of folder)


# API

If you want to use the api, expose a function like this:

```
const LunarTearAPI* g_api;
LT_PluginHandle g_handle;

extern "C" __declspec(dllexport) void LunarTearPluginInit(const LunarTearAPI* api, LT_PluginHandle handle) {

    g_api = api;
    g_handle = handle;

    // Setup your mod here

}
```

The api struct contains pointers to the data and functions.


## Invoking

```
typedef void (*LT_UpdateFunc)(void* userData);
void (*QueuePhaseUpdateTask)(LT_PluginHandle handle, LT_UpdateFunc pFunc, void* userData);
```

Queue a task to be invoked on the main thread, in the phase update loop. Stale commands are invalidated after a few seconds. The update loop is exectured 24 times a second, when in gameplay, (no loading screens or main menu) 


## Logging

```
void Log(LT_PluginHandle handle, LT_LogLevel level, const char* message);
```

Logs a null terminated string using the loaders logging system. The different levels are LT_LOG_INFO, LT_LOG_VERBOSE, LT_LOG_WARNING, LT_LOG_ERROR, LT_LOG_LUA.

## Config

If you want you can place a `config.ini` file in the mods root directory. You can access them with:

```
int Config_GetString(LT_PluginHandle handle, const char* section, const char* key, const char* default_value, char* out_buffer, uint32_t buffer_size);
long Config_GetInteger(LT_PluginHandle handle, const char* section, const char* key, long default_value);
double Config_GetReal(LT_PluginHandle handle, const char* section, const char* key, double default_value);
bool Config_GetBoolean(LT_PluginHandle handle, const char* section, const char* key, bool default_value);
```

Config_GetString returns the number of characters written to out_buffer on success, or -1 on error.

## Hooking

Wrapper around minhook.

```
LT_HookStatus Hook_Create(void* pTarget, void* pDetour, void** ppOriginal);
LT_HookStatus Hook_Enable(void* pTarget);
```


## Scripting

```
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

typedef void (*LT_ScriptExecutionCallbackFunc)(const LT_LuaResult* result, void* userData);

void (*Lua_QueuePhaseScriptExecution)(
    LT_PluginHandle handle,
    const char* script,
    LT_ScriptExecutionCallbackFunc callback,
    void* userData
);
```

Queue a script to be executed in the phase lua state. Thread safe. You must handle safely casting to your desired type.

```
void Lua_QueuePhaseScriptCall(LT_PluginHandle handle, const char* function_name);
```

Don't use this, use Lua_QueuePhaseScriptExecution instead. A thread safe way to call script functions. No way to pass arguments like this. If you want to call a game function with arguments, create your own wrapper around it using script injection and fetch arguments using a binding.

## Custom bindings


You can register custom bindings to be called from game scripts. You can write a binding based on standard lua, but i reccomend using the games handling like this:

```

void c_TestFunc(ScriptState* scriptState) {
    void* pArg1 = api->game->GetArgumentPointer(scriptState->argBuffer, 0);
    char* str = api->game->GetArgumentString(pArg1);
    void* pArg2 = api->game->GetArgumentPointer(scriptState->argBuffer, 1);
    float num = api->game->GetArgumentFloat(pArg2);

    api->Log(g_handle, LT_LOG_INFO, str);

    float num2 = num * 2;

    api->game->SetArgumentFloat(scriptState->returnBuffer, num2);
    scriptState->returnArgCount = 1;

}

void Binding_TestFunc(Lua_State* lua_state) { 
    phaseBindingDispatcher(lua_state, &c_TestFunc);
}

```

register it using:
```       
bool ret = Lua_RegisterCFunc(g_handle, "_TestFunc", &Binding_TestFunc);
```

and call it from lua like:
```
num = _TestFunc("Hello," 5)
```


## Mod loader integration

```
bool (*IsModActive)(LT_PluginHandle handle, const char* mod_name);
bool (*IsPluginActive)(LT_PluginHandle handle, const char* plugin_name);
```

You can use this for compatibility with other mods

```
int (*GetModDirectory)(LT_PluginHandle handle, const char* mod_name, char* out_buffer, uint32_t buffer_size);
```

Use a manifest to define a mod name and this to access resources in your mod directory, as users may change the folder name for conflict resolution



## Versioning

```
const char* (*GetVersionString)(void);
uint32_t(*GetVersionMajor)(void);
uint32_t(*GetVersionMinor)(void);
uint32_t(*GetVersionPatch)(void);
```