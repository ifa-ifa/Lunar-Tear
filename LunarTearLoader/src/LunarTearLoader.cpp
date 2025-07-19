#include <Windows.h>
#include <MinHook.h>
#include <string>
#include "Tex.h"
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <set>
#include "Logger.h"
#include <iterator>
#include <mutex>

using enum Logger::LogCategory;

extern "C" uintptr_t g_processBaseAddress = (uintptr_t)GetModuleHandle(NULL);


static std::map<std::string, std::string> g_resolvedFileMap;
static std::mutex g_fileMapMutex;


void* LoadLooseFile(const char* filename, size_t& out_size) {
    static std::map<std::string, std::vector<char>> file_cache;
    static std::mutex cache_mutex;

    std::string requested_file(filename);
    std::replace(requested_file.begin(), requested_file.end(), '\\', '/');

    std::string full_path;
    {
        const std::lock_guard<std::mutex> lock(g_fileMapMutex);
        auto it = g_resolvedFileMap.find(requested_file);
        if (it == g_resolvedFileMap.end()) {
            out_size = 0;
            return nullptr; 
        }
        full_path = it->second;
    }

    const std::lock_guard<std::mutex> lock(cache_mutex);

    if (file_cache.count(full_path)) {
        out_size = file_cache[full_path].size();
        return file_cache[full_path].data();
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) { 
        Logger::Log(Error) << "Error: Could not open resolved file: " << full_path;
        out_size = 0;
        return nullptr;
    }

    file_cache[full_path] = std::vector<char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
    out_size = file_cache[full_path].size();
    return file_cache[full_path].data();
}

extern "C" void* HandleSettbllHook(char* stbl_filename, void* STBL_data) {
    Logger::Log(FileInfo) <<  "stbl hook triggered for file: " << stbl_filename;
    size_t file_size = 0;
    void* loose_stbl_data = LoadLooseFile(stbl_filename, file_size);
    if (loose_stbl_data) {
        Logger::Log(Info) << " Found loose file";
        return loose_stbl_data;
    }
    return STBL_data;
}

extern "C" void HandleLubHook(char* lub_filename, void** p_lub_data, int* p_lub_size) {
    Logger::Log(FileInfo) << "LUB hook triggered for file: " << lub_filename;
    std::filesystem::path script_path(lub_filename);
    if (script_path.extension() != ".lub") {
        return;
    }
    script_path.replace_extension(".lua");
    std::string loose_source_path_str = script_path.string();
    size_t loose_source_size = 0;
    void* loose_source_data = LoadLooseFile(loose_source_path_str.c_str(), loose_source_size);
    if (loose_source_data) {
        Logger::Log(Info) << "Found loose Lua source: " << loose_source_path_str;
        *p_lub_data = loose_source_data;
        *p_lub_size = static_cast<int>(loose_source_size);
    }
}

void ScanModsAndResolveConflicts() {
    const std::string mods_root = "LunarTear/mods/";
    if (!std::filesystem::exists(mods_root) || !std::filesystem::is_directory(mods_root)) {
        Logger::Log(Info) << "Mods directory not found, skipping mod scan";
        return;
    }

    Logger::Log(Info) << "Scanning for mods...\n";

    // Step 1: Discover all files provided by all mods
    std::map<std::string, std::vector<std::string>> file_to_mods_map;
    for (const auto& mod_entry : std::filesystem::directory_iterator(mods_root)) {
        if (!mod_entry.is_directory()) continue;

        std::string mod_name = mod_entry.path().filename().string();
        Logger::Log(Verbose) << "Found mod: " << mod_name;

        for (const auto& file_entry : std::filesystem::recursive_directory_iterator(mod_entry.path())) {
            if (!file_entry.is_regular_file()) continue;

            std::string relative_path = std::filesystem::relative(file_entry.path(), mod_entry.path()).string();
            std::replace(relative_path.begin(), relative_path.end(), '\\', '/');
            file_to_mods_map[relative_path].push_back(mod_name);
        }
    }

    std::set<std::string> disabled_mods;
    std::string conflict_report = "";

    for (auto const& [file, mods] : file_to_mods_map) {
        if (mods.size() > 1) {
            std::vector<std::string> sorted_mods = mods;
            std::sort(sorted_mods.begin(), sorted_mods.end());

            conflict_report += "Conflict for file: " + file + "\n";
            conflict_report += "  - Winner (loaded): " + sorted_mods[0] + "\n";

            for (size_t i = 1; i < sorted_mods.size(); ++i) {
                conflict_report += "  - Loser (disabled): " + sorted_mods[i] + "\n";
                disabled_mods.insert(sorted_mods[i]);
            }
            conflict_report += "\n";
        }
    }

    // Step 3: Build the final resolved file map, excluding files from disabled mods
    {
        const std::lock_guard<std::mutex> lock(g_fileMapMutex);
        g_resolvedFileMap.clear();

        for (auto const& [file, mods] : file_to_mods_map) {
            std::string winner_mod = mods[0];
            if (mods.size() > 1) {
                winner_mod = *std::min_element(mods.begin(), mods.end());
            }

            if (disabled_mods.find(winner_mod) == disabled_mods.end()) {
                std::filesystem::path full_path = std::filesystem::path(mods_root) / winner_mod / file;
                g_resolvedFileMap[file] = full_path.string();
                Logger::Log(Verbose) << "Mapping " << file << " -> " << g_resolvedFileMap[file];
            }
        }
    }

    if (!conflict_report.empty()) {
        std::string final_message = "LunarTear found mod conflicts\n\n";
        final_message += "The alphabetically first mod is loaded. All files from other conflicting mods are disabled.\n\n";
        final_message += conflict_report;

        MessageBoxA(GetActiveWindow(), final_message.c_str(), "LunarTear Mod Conflict", MB_OK | MB_ICONWARNING | MB_APPLMODAL);
    }

    Logger::Log(Info) << "Mod scan complete";
}

extern "C" void SettbllAssemblyStub();
extern "C" void LubAssemblyStub();

extern "C" void* pTrampoline_Settbll = nullptr;
extern "C" void* pTrampoline_Lub = nullptr;
extern "C" void* pTrampoline_Tex = nullptr;

typedef uint64_t(__fastcall* TexHook)(tpgxResTexture*, void*, void*);
TexHook TexHook_hooked = TexHook(g_processBaseAddress + 0x7dc820);
TexHook TexHook_original;

uint64_t TexHook_detoured(tpgxResTexture* tex, void* param_2, void* param_3) {
    if (tex && tex->name && tex->bxonHeader && tex->bxonAssetHeader) {
        Logger::Log(FileInfo) << "Hooked texture: " << std::string(tex->name)
            << " | Original Format: " << XonFormatToString(tex->bxonAssetHeader->XonSurfaceFormat)
            << " | Original Mipmap Count: " << tex->bxonAssetHeader->numMipSurfaces;


        std::filesystem::path tex_path(tex->name);
        tex_path.replace_extension(".dds");

        size_t ddsFileSize = 0;
        void* ddsFileData = LoadLooseFile(tex_path.string().c_str(), ddsFileSize);

        if (ddsFileData) {
            Logger::Log(Info) << "  -> Found loose file: " << tex_path.string();

            if (ddsFileSize < sizeof(DDS_HEADER)) {
                Logger::Log(Error) << "  -> Error: DDS file is too small to be valid.";
                return TexHook_original(tex, param_2, param_3);
            }
            DDS_HEADER* ddsHeader = static_cast<DDS_HEADER*>(ddsFileData);
            if (ddsHeader->magic != 0x20534444) {
                Logger::Log(Error) << "  -> Error: Invalid DDS magic.";
                return TexHook_original(tex, param_2, param_3);
            }

            XonSurfaceDXGIFormat finalFormat = UNKNOWN;
            void* pixelDataStart = nullptr;
            size_t pixelDataSize = 0;

            if (ddsHeader->ddspf_dwFourCC == '01XD') {
                Logger::Log(Verbose) << "  -> DX10 header detected.";
                if (ddsFileSize < sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)) {
                    Logger::Log(Error) << "  -> Error: DX10 file is missing the extended header.";
                    return TexHook_original(tex, param_2, param_3);
                }
                DDS_HEADER_DXT10* dx10Header = (DDS_HEADER_DXT10*)((char*)ddsFileData + sizeof(DDS_HEADER));
                finalFormat = DXGIFormatToXonFormat(dx10Header->dxgiFormat);
                pixelDataStart = (char*)dx10Header + sizeof(DDS_HEADER_DXT10);
                pixelDataSize = ddsFileSize - (sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10));
            }
            else {
                Logger::Log(Verbose) << "  -> Legacy DDS header detected.";
                finalFormat = FourCCToXonFormat(ddsHeader->ddspf_dwFourCC);
                pixelDataStart = (char*)ddsHeader + sizeof(DDS_HEADER);
                pixelDataSize = ddsFileSize - sizeof(DDS_HEADER);
            }

            if (finalFormat == UNKNOWN) {
                Logger::Log(Error) << "  -> Error: Unsupported DDS pixel format in loose file.";
                return TexHook_original(tex, param_2, param_3);
            }

            if (ddsHeader->dwMipMapCount > tex->bxonAssetHeader->numMipSurfaces) {
                Logger::Log(Error) << "  -> Error: Loose DDS has more mipmaps (" << ddsHeader->dwMipMapCount
                    << ") than original texture (" << tex->bxonAssetHeader->numMipSurfaces
                    << "). Cannot modify in-place. Reverting to original textures";

                return TexHook_original(tex, param_2, param_3);
            }

      
            tex->texData = pixelDataStart; // game uses arena allocator, unlikely to try free this pointer
            tex->texDataSize = pixelDataSize;
            tex->bxonAssetHeader->texWidth = ddsHeader->dwWidth;
            tex->bxonAssetHeader->texHeight = ddsHeader->dwHeight;
            tex->bxonAssetHeader->texSize = pixelDataSize;
            tex->bxonAssetHeader->XonSurfaceFormat = finalFormat;
            tex->bxonAssetHeader->numMipSurfaces = ddsHeader->dwMipMapCount;

            mipSurface* pMipSurfaces = (mipSurface*)((uintptr_t)&tex->bxonAssetHeader->offsetToMipSurfaces + tex->bxonAssetHeader->offsetToMipSurfaces);

            uint32_t currentOffset = 0;
            uint32_t currentWidth = ddsHeader->dwWidth;
            uint32_t currentHeight = ddsHeader->dwHeight;

            for (uint32_t i = 0; i < ddsHeader->dwMipMapCount; i++) {
                bool is_8_byte_format = (finalFormat == BC1_UNORM || finalFormat == BC1_UNORM_SRGB || finalFormat == BC4_UNORM);
                uint32_t bytesPerBlock = is_8_byte_format ? 8 : 16;
                uint32_t mipWidth = max(1u, currentWidth);
                uint32_t mipHeight = max(1u, currentHeight);
                uint32_t mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * bytesPerBlock;

                pMipSurfaces[i].offset = currentOffset;
                pMipSurfaces[i].size = mipSize;
                pMipSurfaces[i].width = mipWidth;
                pMipSurfaces[i].height = mipHeight;

                currentOffset += mipSize;
                if (currentWidth > 1) currentWidth /= 2;
                if (currentHeight > 1) currentHeight /= 2;
            }
        }
    }
    return TexHook_original(tex, param_2, param_3);
}

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal) {
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

DWORD WINAPI InitializeHooks(LPVOID lpParameter) {
    Logger::Init();
    if (!g_processBaseAddress) { 
        Logger::Log(Error) << "Could not initialize Lunar Tear";
        return FALSE; 
    }

    ScanModsAndResolveConflicts();

    if (MH_Initialize() != MH_OK) { 
        Logger::Log(Error) << "Could not initialize minhook";

        return FALSE; 
    }

    void* pSettbllTarget = (void*)(g_processBaseAddress + 0x4154d8);
    void* pLubTarget = (void*)(g_processBaseAddress + 0x4153f8);

    if (MH_CreateHook(pLubTarget, &LubAssemblyStub, &pTrampoline_Lub) != MH_OK) {
        Logger::Log(Error) << "Could not create lua hook";
        return FALSE;
    }
    if (MH_CreateHook(pSettbllTarget, &SettbllAssemblyStub, &pTrampoline_Settbll) != MH_OK) {
        Logger::Log(Error) << "Could not create table hook";
        return FALSE;
    }
    if (MH_CreateHookEx(TexHook_hooked, &TexHook_detoured, &TexHook_original)) {
        Logger::Log(Error) << "Could not create texture hook";
        return FALSE;
    }
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        Logger::Log(Error) << "Could not enable hooks";
        return FALSE;
    }

    Logger::Log(Verbose) << "Hooks initialized";
    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, InitializeHooks, hModule, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        MH_Uninitialize();
    }
    return TRUE;
}

static_assert(offsetof(tpgxResTexture, name) == 0x40, "tpgxResTexture::name offset is wrong");
static_assert(offsetof(tpgxResTexture, bxonHeader) == 0x90, "tpgxResTexture::bxonHeader offset is wrong");