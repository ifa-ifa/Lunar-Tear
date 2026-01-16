#define NOMINMAX
#include "Hooks.h"
#include "Game/TextureFormats.h"
#include "ModLoader.h"
#include "Common/Settings.h"
#include "Common/Dump.h"
#include "Common/Logger.h"
#include "Game/Globals.h"
#include <crc32c/crc32c.h>
#include <MinHook.h>
#include <filesystem>
#include <replicant/dds.h>
#include <replicant/tpGxTexHead.h>
#include "Game/Weapons.h"

namespace {
    typedef void(__fastcall* WeaponLoad_t)();
    WeaponLoad_t WeaponLoad_original = nullptr;

}

void WeaponLoad_detoured() {

    WeaponLoad_original();

    InitWeaponSystem();

}

bool InstallWeaponLoadHooks() {

    void* weaponLoad_target = (void*)(g_processBaseAddress + 0x408b80);

    InstallHook(weaponLoad_target, &WeaponLoad_detoured, &WeaponLoad_original, "WeaponLoad hook");

    return true;
}