#include "Hooks.h"

#include "ModLoader.h"
#include "Common/Settings.h"
#include "Common/Logger.h"
#include "Common/Dump.h"
#include <MinHook.h>
#include <filesystem>
#include<vector>

using enum Logger::LogCategory;

extern "C" {
    void PhaseScriptStub();
    void* PhaseScriptTrampoline = nullptr;

    void LibScriptStub();
    void* LibScriptTrampoline = nullptr;

    void GameScriptStub();
    void* GameScriptTrampoline = nullptr;

    void RootScriptStub();
    void* RootScriptTrampoline = nullptr;

}

#ifdef __INTELLISENSE__
void PhaseScriptStub() {}
void LibScriptStub() {}
void GameScriptStub() {}
void RootScriptStub() {}
#endif 

namespace {
    uintptr_t g_processBaseAddress = (uintptr_t)GetModuleHandle(NULL);
    void* PhaseScriptTarget = (void*)(g_processBaseAddress + 0x4153f8);
    void* LibScriptTarget = (void*)(g_processBaseAddress + 0x415581);
    void* GameScriptTarget = (void*)(g_processBaseAddress + 0x414795);
    void* RootScriptTarget = (void*)(g_processBaseAddress + 0x4141fd);


}

extern "C" void HandleLubHook(char* lub_filename, void** p_lub_data, int* p_lub_size) {
    Logger::Log(FileInfo) << "Hooked Script: " << lub_filename;

    if (Settings::Instance().DumpScripts) {
        Logger::Log(Info) << "Dumping file: " << lub_filename;

        dumpScript(lub_filename, std::vector<char>((char*)*p_lub_data, (char*)*p_lub_data + *p_lub_size));
    }

    std::filesystem::path script_path(lub_filename);
    if (script_path.extension() != ".lub") {
        return;
    }
    script_path.replace_extension(".lua");
    std::string loose_source_path_str = script_path.string();
    size_t loose_source_size = 0;
    void* loose_source_data = LoadLooseFile(loose_source_path_str.c_str(), loose_source_size);



    if (!loose_source_data) {
        return;
    }

    Logger::Log(Info) << "Found loose Lua source file: " << loose_source_path_str;
    

    *p_lub_data = loose_source_data;
    *p_lub_size = static_cast<int>(loose_source_size);
    
}

bool InstallScriptHooks() {
    if (MH_CreateHook(PhaseScriptTarget, &PhaseScriptStub, &PhaseScriptTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create phase script hook";
        return false;
    }
    if (MH_CreateHook(LibScriptTarget, &LibScriptStub, &LibScriptTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create lib script hook";
        return false;
    }
    if (MH_CreateHook(GameScriptTarget, &GameScriptStub, &GameScriptTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create lib script hook";
        return false;
    }  
    if (MH_CreateHook(RootScriptTarget, &RootScriptStub, &RootScriptTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create lib script hook";
        return false;
    }

    return true;
}