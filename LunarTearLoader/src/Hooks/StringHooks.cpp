#include "Hooks.h"
#include "Game/Globals.h"
#include "Game/Strings.h"
#include <mutex>

namespace {

#pragma pack(push, 1)
    struct StringLocaleEntry {
        int id;
        char padding[4];
        const char* str;
    };
#pragma pack(pop) 

    typedef const char*(__fastcall* GetLocaleString_t)(StringLocaleEntry* localeTable, int id);
    GetLocaleString_t GetLocaleStringOriginal = nullptr;

}

const char* GetLocaleStringDetoured(StringLocaleEntry* localeTable, int id) {

    if (id < LTStringRangeMin) { return GetLocaleStringOriginal(localeTable, id); }
    return getLTString(id);

}

bool InstallStringHooks() {

    void* GetLocaleStringTarget = (void*)(g_processBaseAddress + 0xd71d0);
    InstallHook(GetLocaleStringTarget, &GetLocaleStringDetoured, &GetLocaleStringOriginal, "Get String");
    return true;
}