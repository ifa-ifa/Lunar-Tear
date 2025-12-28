#include "Hooks/Hooks.h"
#include "Game/Globals.h"
#include "Common/Logger.h"
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

static bool patching_attempted = false;
static bool patching_succeeded = false;

void decompressArchive_detoured(tpFndTextFormat256* basePath, const char* archiveName, void** outbuffer, void* allocator) {

    std::string archiveNameStr(archiveName);
    std::string basePathStr(basePath->buffer);

    if (basePathStr + archiveNameStr == "data/info.arc") {

        if (!patching_attempted) {
            Logger::Log(Verbose) << "First request for 'info.arc' detected. Waiting for mod scan...";

            int total_timer = 0;
            while (!VFS_ready) {

                Sleep(100);
                total_timer += 100;
                if (total_timer > 5000) {
                    Logger::Log(Error) << "Timout while waiting for mod scan to finish. Cancelling VFS hook";
                    patching_attempted = true;
                    return decompressArchive_original(basePath, archiveName, outbuffer, allocator);
                    
                }
            }

            patching_succeeded = GeneratePatchedIndex();
            patching_attempted = true;
        }

        if (patching_succeeded && !g_patched_archive_mods.empty()) {
            Logger::Log(Info) << "Redirecting 'info.arc' to patched 'LunarTear/LunarTear.arc'";

            tpFndTextFormat256 newBasePath = *basePath;
            strcpy_s(newBasePath.buffer, sizeof(newBasePath.buffer), "LunarTear/");

            decompressArchive_original(&newBasePath, "LunarTear.arc", outbuffer, allocator);
            return; 
        }
    }
    else if (!patching_attempted) {
        Logger::Log(Error) << "VFS hook missed, hook likely attatched too late. mods using archives will not work";
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