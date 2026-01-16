#include "Common/Save.h"
#include "Hooks/Hooks.h"
#include "Game/Globals.h"
#include "Common/Logger.h"
#include "VFS/ArchivePatcher.h"
#include "ModLoader.h"
#include <string>
#include <filesystem>
#include <format>
#include <chrono>

using enum Logger::LogCategory;

/*
Slots 0-2: Nier
Slots 3-5: Kaine
Slot 6: Most recently loaded save is copied into here upon completing ending D, reloaded afterwards.

*/

namespace {

    // The aim of these hooks is to shadow the games movment of save data to allow modders to 
    //   consistenly save data without steganography


    // loads the given slot into active data
    typedef bool(__fastcall* loadPlayerData_t)(void* gamedata, uint32_t slot);
    loadPlayerData_t loadPlayerData_original = nullptr;

    // saves active data to given slot
    typedef void(__fastcall* savePlayerData_t)(void* gamedata, uint32_t slot, bool param_3, bool param_4);
    savePlayerData_t savePlayerData_original = nullptr;

    // loads slot 7 into active data
    typedef void(__fastcall* loadFrom7thSlot_t)(void* param_1, uint8_t param_2);
    loadFrom7thSlot_t loadFrom7thSlot_original = nullptr;

    // loads given slot (not active data!) into slot 7
    // Also alters the bitmask to invalidate slots 0-2
    typedef void(__fastcall* saveTo7thSlot_t)(void* gamedata, uint32_t slot);
    saveTo7thSlot_t saveTo7thSlot_original = nullptr;


    bool loadPlayerData_detoured(void* gamedata, uint32_t slot) {

        bool res = loadPlayerData_original(gamedata, slot);
        if (!res) {
            return false;
        }
        
        Save::loadActiveDataFromDisk(slot);
        Save::exePostLoadCallbacks();

        return true;
    }

    void savePlayerData_detoured(void* gamedata, uint32_t slot, bool param_3, bool param_4) {
    
        Save::exePreSaveCallbacks();

        savePlayerData_original(gamedata, slot, param_3, param_4);
        Save::saveActiveDataToDisk(slot);

        Save::exePostSaveCallbacks();
    }
    void loadFrom7thSlot_detoured(void* param_1, uint8_t param_2) {
    
        loadFrom7thSlot_original(param_1,   param_2);
        Save::loadActiveDataFromDisk(6);

        Save::exePostLoadCallbacks();

    }
    void saveTo7thSlot_detoured(void* gamedata, uint32_t slot) {
    
        // This is rough, since we are copying serialised data it should already be vanilla compatible,
        //   but users may be confusted that their callbacks are not being called here

        saveTo7thSlot_original(gamedata, slot);
        Save::copySlot(slot, 6);

        // The game here clears slots 1-3
        Save::clearSlot(0);
        Save::clearSlot(1);
        Save::clearSlot(2);

    }
}




bool InstallSaveHooks() {

    void* loadPlayerData_target = (void*)(g_processBaseAddress + 0x3be800);
    void* savePlayerData_target = (void*)(g_processBaseAddress + 0x3be880);
    void* loadFrom7thSlot_target = (void*)(g_processBaseAddress + 0x4b1940);
    void* saveTo7thSlot_target = (void*)(g_processBaseAddress + 0x3bf700);


    InstallHook(loadPlayerData_target, &loadPlayerData_detoured, &loadPlayerData_original, "loadPlayerData");
    InstallHook(savePlayerData_target, &savePlayerData_detoured, &savePlayerData_original, "savePlayerData");
    InstallHook(loadFrom7thSlot_target, &loadFrom7thSlot_detoured, &loadFrom7thSlot_original, "loadFrom7thSlot");
    InstallHook(saveTo7thSlot_target, &saveTo7thSlot_detoured, &saveTo7thSlot_original, "saveTo7thSlot");

    return true;
}