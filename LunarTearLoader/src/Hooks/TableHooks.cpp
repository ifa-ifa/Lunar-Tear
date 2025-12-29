#include "Hooks.h"

#include "ModLoader.h"
#include "Common/Settings.h"
#include "Game/Globals.h"
#include "Common/Logger.h"
#include "Common/Dump.h"
#include <string>
#include <MinHook.h>

using enum Logger::LogCategory;

extern "C" {
    void PhaseTableStub();
    void* PhaseTableTrampoline = nullptr;

    void DroptableTableStub();
    void* DroptableTableTrampoline = nullptr;

    void GameTableStub();
    void* GameTableTrampoline = nullptr;
}

#ifdef __INTELLISENSE__
void PhaseTableStub() {}
void DroptableTableStub() {}
void GameTableStub() {}
#endif 


extern "C" void* HandleSettbllHook(char* stbl_filename, void* STBL_data) {
    Logger::Log(FileInfo) << "Hooked Table: " << stbl_filename;

    if (Settings::Instance().DumpTables) {
        dumpTable(std::string(stbl_filename), (char*)STBL_data);
    }

    size_t file_size = 0;
    void* loose_stbl_data = LoadLooseFile(stbl_filename, file_size);
    if (loose_stbl_data) {
        Logger::Log(Info) << "Found loose table file: " << stbl_filename;
        return loose_stbl_data;
    }
    return STBL_data;
}

bool InstallTableHooks() {

    void* PhaseTableTarget = (void*)(g_processBaseAddress + 0x4154d8);
    void* DroptableTableTarget = (void*)(g_processBaseAddress + 0x4148b3);
    void* GameTableTarget = (void*)(g_processBaseAddress + 0x414813);

    if (MH_CreateHook(PhaseTableTarget, &PhaseTableStub, &PhaseTableTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create phase table hook";
        return false;
    }
    if (MH_CreateHook(DroptableTableTarget, &DroptableTableStub, &DroptableTableTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create droptable table hook";
        return false;
    }   
    if (MH_CreateHook(GameTableTarget, &GameTableStub, &GameTableTrampoline) != MH_OK) {
        Logger::Log(Error) << "Could not create game table hook";
        return false;
    }

    bool hooks_ok = true;
    hooks_ok &= MH_EnableHook(PhaseTableTarget);
    hooks_ok &= MH_EnableHook(DroptableTableTarget);
    hooks_ok &= MH_EnableHook(GameTableTarget);
    if (!hooks_ok) {
        Logger::Log(Error) << "Could not enable table inject hooks";
        return false;

    }

    return true;
}