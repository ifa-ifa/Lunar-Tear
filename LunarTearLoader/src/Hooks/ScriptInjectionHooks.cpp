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

    void PostLibStub();
    void* PostLibTrampoline = nullptr;

    void PostRootStub();
    void* PostRootTrampoline = nullptr;

    void PostGameStub();
    void* PostGameTrampoline = nullptr;

}

extern "C" {

    void InjectPostPhaseScripts(const char* phaseScriptName);
    void InjectPostLibScripts();
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
    auto scripts = GetInjectionScripts(point);
    std::lock_guard<std::mutex> lock(API::s_lua_binding_mutex);

    // The game re-registers bindings on every single phase load. We will do as the game does.

    const auto& plugin_bindings_pairs = API::GetPluginLuaBindings();
    if (!plugin_bindings_pairs.empty()) {
        std::vector<LuaCBinding> bindings_with_terminator;
        bindings_with_terminator.reserve(plugin_bindings_pairs.size() + 1);

        for (const auto& pair : plugin_bindings_pairs) {
            bindings_with_terminator.push_back({ pair.first.c_str(), (void*)pair.second });
        }
        bindings_with_terminator.push_back({ NULL, NULL }); // NULL terminator

        Logger::Log(Info) << "Registering " << plugin_bindings_pairs.size() << " Lua function(s) from plugins...";
        ScriptManager_registerBindings(&(ctxMn->scriptManager), bindings_with_terminator.data());
    }

    int64_t ret = ScriptManager_registerBindings(&(ctxMn->scriptManager), GetCoreBindings());
    Logger::Log(Info) << "regisetered bindings, code:" << ret;

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

extern "C" void InjectPostPhaseScripts(const char* phaseScriptName) {

    if (!phaseScriptName) return;

    std::filesystem::path p(phaseScriptName);
    if (p.extension() != ".lub") return;

    p.replace_extension(".lua");
    std::string script_name_lua = p.filename().string();

    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, "post_phase/_any.lua");
    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, "post_phase/" + script_name_lua);
}

extern "C" void InjectPostLibScripts() {
    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, "post_lib/_any.lua");
    ExecuteScriptsForPoint((ScriptContextManager*)phaseScriptManager, "post_lib/__libnier__.lua");
}


extern "C" void InjectPostRootScripts() {
    ExecuteScriptsForPoint((ScriptContextManager*)rootScriptManager, "post_root/_any.lua");
    ExecuteScriptsForPoint((ScriptContextManager*)rootScriptManager, "post_root/__root__.lua");
}

extern "C" void InjectPostGameScripts() {
    ExecuteScriptsForPoint((ScriptContextManager*)gameScriptManager, "post_game/_any.lua");
    ExecuteScriptsForPoint((ScriptContextManager*)gameScriptManager, "post_game/__game__.lua");
}


#ifdef __INTELLISENSE__
void PostPhaseStub() {}
void PostLibStub() {}
void PostGameStub() {}
void PostRootStub() {}
#endif 


bool InstallScriptInjectHooks() {

    // These targets are right after the call to ScriptManager_LoadAndRunBuffer for each type of cripts,
    // they allow us to inject scripts right after and hook the original
    void* PostPhaseTarget = (void*)(g_processBaseAddress + 0x4153fd);
    void* PostLibTarget = (void*)(g_processBaseAddress + 0x415586);

    void* PostGameTarget = (void*)(g_processBaseAddress + 0x41479a);
    void* PostRootTarget = (void*)(g_processBaseAddress + 0x414202);


    if (MH_CreateHook(PostPhaseTarget, &PostPhaseStub, &PostPhaseTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create post phase hook";
        return false;
    }
    if (MH_CreateHook(PostLibTarget, &PostLibStub, &PostLibTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create post lib hook";
        return false;
    }
    if (MH_CreateHook(PostGameTarget, &PostGameStub, &PostGameTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create post game hook";
        return false;
    }
    if (MH_CreateHook(PostRootTarget, &PostRootStub, &PostRootTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create post root hook";
        return false;
    }

	MH_STATUS status;
    status = MH_EnableHook(PostPhaseTarget);
    if (status != MH_OK) {
        Logger::Log(Error) << "Could not enable post phase hook: " << (int)status;
        return false;
	}
    status = MH_EnableHook(PostLibTarget);
    if (status != MH_OK) {
        Logger::Log(Error) << "Could not enable post lib hook: " << (int)status;
        return false;
	}
    status = MH_EnableHook(PostGameTarget);
    if (status != MH_OK) {
        Logger::Log(Error) << "Could not enable post game hook: " << (int)status;
        return false;
    }
    status = MH_EnableHook(PostRootTarget);
    if (status != MH_OK) {
        Logger::Log(Error) << "Could not enable post root hook: " << (int)status;
        return false;
    }


    return true;
}
