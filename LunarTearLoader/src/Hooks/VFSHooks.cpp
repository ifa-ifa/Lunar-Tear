#include "Hooks/Hooks.h"
#include "Game/Globals.h"
#include "Common/Logger.h"
#include "VFS/patchInfo.h"
#include "VFS/ArchivePatcher.h"
#include "ModLoader.h"
#include <string>
#include <filesystem>

using enum Logger::LogCategory;

struct tpFndTextFormat256 {
    void* vtable;
    char buffer[256];
};

namespace {
    typedef void(__fastcall* decompressArchive_t)(tpFndTextFormat256* basePath, const char* archiveName, void** outbuffer, void* allocator);
    decompressArchive_t decompressArchive_original = nullptr;
}

void decompressArchive_detoured(tpFndTextFormat256* basePath, const char* archiveName, void** outbuffer, void* allocator) {
    std::string archiveNameStr(archiveName);
    std::string basePathStr(basePath->buffer);


    Logger::Log(Verbose) << "Hooked decompressArchive: " << "basePath / archiveName";

    if (basePathStr + archiveNameStr ==  "data/info.arc") {

        int timer = 0;
        while (infopatch_status == INFOPATCH_INCOMPLETE) {
            if (timer > 15000) {
                Logger::Log(Error) << "Timeout waiting for archive patcher. VFS mods will not be loaded.";
                infopatch_status = INFOPATCH_ERROR;
                break;
            }
            Sleep(100);
            timer += 100;
        }
        if (infopatch_status == INFOPATCH_READY && !g_patched_archive_mods.empty()) {
            Logger::Log(Info) << "Redirecting 'info.arc' to patched 'LunarTear/LunarTear.arc'";

            tpFndTextFormat256 newBasePath = *basePath;
            strcpy_s(newBasePath.buffer, sizeof(newBasePath.buffer), "LunarTear/");
            decompressArchive_original(&newBasePath, "LunarTear.arc", outbuffer, allocator);
            return; 
        }
        

        // TODO: add more error logging
    }

    decompressArchive_original(basePath, archiveName, outbuffer, allocator);
}


bool InstallVFSHooks() {
    void* decompressArchive_target = (void*)(g_processBaseAddress + 0x08ed730);
    if (MH_CreateHookEx(decompressArchive_target, &decompressArchive_detoured, &decompressArchive_original) != MH_OK) {
        return false;
    }
    return true;
}