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
    void LubAssemblyStub();
    void* pTrampoline_Lub = nullptr;
}

#ifdef __INTELLISENSE__
void LubAssemblyStub() {}
#endif 

namespace {
    uintptr_t g_processBaseAddress = (uintptr_t)GetModuleHandle(NULL);
    void* pLubTarget = (void*)(g_processBaseAddress + 0x4153f8);
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

bool InstallScriptHook() {
    if (MH_CreateHook(pLubTarget, &LubAssemblyStub, &pTrampoline_Lub) != MH_OK) {
        Logger::Log(Error) << "Could not create lua hook";
        return false;
    }
    return true;
}