#include "Hooks.h"

#include "ModLoader.h"
#include "Common/Settings.h"
#include "Common/Logger.h"
#include "Common/Dump.h"
#include <string>
#include <MinHook.h>

using enum Logger::LogCategory;

extern "C" {
    void SettbllAssemblyStub();
    void* pTrampoline_Settbll = nullptr;
}

#ifdef __INTELLISENSE__
void SettbllAssemblyStub() {}
#endif 

namespace {
    uintptr_t g_processBaseAddress = (uintptr_t)GetModuleHandle(NULL);
    void* pSettbllTarget = (void*)(g_processBaseAddress + 0x4154d8);
}

extern "C" void* HandleSettbllHook(char* stbl_filename, void* STBL_data) {
    Logger::Log(FileInfo) << "Hooked Table: " << stbl_filename;

    if (Settings::Instance().DumpTables) {
        dumpTable(std::string(stbl_filename), (char*)STBL_data);
    }

    size_t file_size = 0;
    void* loose_stbl_data = LoadLooseFile(stbl_filename, file_size);
    if (loose_stbl_data) {
        Logger::Log(Info) << " Found loose table file: " << stbl_filename;
        return loose_stbl_data;
    }
    return STBL_data;
}

bool InstallTableHook() {
    if (MH_CreateHook(pSettbllTarget, &SettbllAssemblyStub, &pTrampoline_Settbll) != MH_OK) {
        Logger::Log(Error) << "Could not create table hook";
        return false;
    }
    return true;
}