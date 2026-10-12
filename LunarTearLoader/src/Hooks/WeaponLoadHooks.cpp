#include "Hooks.h"
#include "Game/Globals.h"
#include "Game/Weapons.h"

namespace {
    typedef void(__fastcall* WeaponLoad_t)();
    WeaponLoad_t WeaponLoad_original = nullptr;

}

// Called once on startup
void WeaponLoad_detoured() {

    WeaponLoad_original();
    InitWeaponSystem();

}

bool InstallWeaponLoadHooks() {

    void* weaponLoad_target = (void*)(g_processBaseAddress + 0x408b80);

    InstallHook(weaponLoad_target, &WeaponLoad_detoured, &WeaponLoad_original, "WeaponLoad hook");

    return true;
}