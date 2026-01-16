#include "Hooks.h"

#include "API/API.h"
#include "ModLoader.h"
#include "Common/Settings.h"
#include "Common/Logger.h"
#include "Game/Functions.h"
#include <mutex> 
#include "Common/Dump.h"
#include "Game/Globals.h"
#include "Lua/CoreBindings.h" 
#include "Lua/LuaCommandQueue.h"
#include <MinHook.h>
#include <filesystem>
#include<vector>  

using enum Logger::LogCategory;

extern "C" {

    void PostPhaseStub();
    void* PostPhaseTrampoline = nullptr;

    void PostRootStub();
    void* PostRootTrampoline = nullptr;

    void PostGameStub();
    void* PostGameTrampoline = nullptr;

}

extern "C" {

    void InjectPostPhaseScripts();
    void InjectPostGameScripts();
    void InjectPostRootScripts();


}


std::string scriptDispatcher = R"(

ififa_LTCore_scriptDispatch = function()

    local command = _ifaifa_LTCore_GetScript()
    if command == nil or command == '' then
        -- This can happen if the queue is processed but there's no script. Just exit.
        return
    end

    local func, compile_err = _ifaifa_LTCon_loadstring(command)

    if not func then
        _ifaifa_LTCore_ReportResult("syntax_error:" .. _LTLua_tostring(compile_err))
        return
    end

    local function_to_execute = function()

        local retval = func()
        local rettype = _LTLua_type(retval)

        if rettype == "number" then
            _ifaifa_LTCore_ReportResult("number:" .. _LTLua_tostring(retval))
        elseif rettype == "string" then
            _ifaifa_LTCore_ReportResult("string:" .. retval)
        elseif rettype == "boolean" then
            -- Report booleans as integers (1 for true, 0 for false)
            _ifaifa_LTCore_ReportResult("number:" .. (_LTLua_tostring(retval) == "true" and "1" or "0"))
        elseif rettype == "nil" then
            _ifaifa_LTCore_ReportResult("nil:nil")
        else
            -- Report an error for unsupported types like tables, functions, etc.
            _ifaifa_LTCore_ReportResult("error_unsupported:" .. rettype)
        end
    end

    local error_handler = function(err_msg)
        _ifaifa_LTCore_ReportResult("runtime_error:" .. _LTLua_tostring(err_msg))
    end

    _LTLua_xpcall(function_to_execute, error_handler)
end

)";

void ExecuteScriptsForPoint(ScriptContextManager* ctxMn, const std::string& point) {
    std::lock_guard<std::mutex> lock(API::s_lua_binding_mutex);

    const auto& plugin_bindings = API::GetPluginLuaBindings();
    const auto& core_bindings = GetCoreBindings();
    auto all_bindings = plugin_bindings;
    all_bindings.insert(all_bindings.end(), core_bindings.begin(), core_bindings.end());
    
    std::vector<LuaCBinding> raw_bindings;
    for (size_t  i = 0; i < all_bindings.size() ; i++) {
        raw_bindings.push_back({ all_bindings[i].first.c_str(), all_bindings[i].second });
    }
    raw_bindings.push_back({ NULL, NULL });

    int64_t ret = ScriptManager_registerBindings(&(ctxMn->scriptManager), raw_bindings.data());

    Logger::Log(Verbose) << "regisetered bindings, code: " << ret;

    auto scripts = GetInjectionScripts(point);

    if (scripts.empty()) {
        return;
    }

    Logger::Log(Info) << "Injecting " << scripts.size() << " script(s) for point '" << point << "'";

    ScriptManager_LoadAndRunBuffer( // internal script for QueuePhaseScriptExecution
        &(ctxMn->scriptManager),
        (void*)scriptDispatcher.data(),
        scriptDispatcher.size()
    );

    for (const auto& script_data : scripts) {
        if (script_data.empty()) continue;
        ret = ScriptManager_LoadAndRunBuffer(
            &(ctxMn->scriptManager),
            (void*)script_data.data(),
            script_data.size()
        );
        Logger::Log(Info) << "Injected scripts, code:" << ret;

    }
}

extern "C" void InjectPostPhaseScripts() {

    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, "_any");
    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, "__phase__");
    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, playerSaveData->current_phase);
    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, "__libnier__");

}


extern "C" void InjectPostRootScripts() {
    ExecuteScriptsForPoint((ScriptContextManager*)rootScriptManager, "_any");
    ExecuteScriptsForPoint((ScriptContextManager*)rootScriptManager, "__root__");
}

extern "C" void InjectPostGameScripts() {
    ExecuteScriptsForPoint((ScriptContextManager*)gameScriptManager, "_any");
    ExecuteScriptsForPoint((ScriptContextManager*)gameScriptManager, "__game__");
}


#ifdef __INTELLISENSE__
void PostPhaseStub() {}
void PostGameStub() {}
void PostRootStub() {}
#endif 


bool InstallScriptInjectHooks() {

    // These targets are right after the call to ScriptManager_LoadAndRunBuffer for each type of scripts,
    // they allow us to inject scripts right after and hook the original
    void* PostPhaseTarget = (void*)(g_processBaseAddress + 0x415586);
    void* PostGameTarget = (void*)(g_processBaseAddress + 0x41479a);
    void* PostRootTarget = (void*)(g_processBaseAddress + 0x414202);

    InstallHook(PostPhaseTarget, &PostPhaseStub, &PostPhaseTrampoline, "Post Phase");
    InstallHook(PostGameTarget, &PostGameStub, &PostGameTrampoline, "Post Game");
    InstallHook(PostRootTarget, &PostRootStub, &PostRootTrampoline, "Post Root");

    return true;
}
