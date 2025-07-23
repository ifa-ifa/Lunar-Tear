#define NOMINMAX
#include "Hooks.h"
#include "Formats/TextureFormats.h"
#include "ModLoader.h"
#include "Common/Settings.h"
#include "Common/Dump.h"
#include "Common/Logger.h"
#include <MinHook.h>
#include <filesystem>

using enum Logger::LogCategory;

namespace {
    typedef uint64_t(__fastcall* TexHook_t)(tpgxResTexture*, void*, void*);
    TexHook_t TexHook_original = nullptr;
    uintptr_t g_processBaseAddress = (uintptr_t)GetModuleHandle(NULL);
    void* TexHook_target = (void*)(g_processBaseAddress + 0x7dc820);

    template <typename T>
    inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal) {
        return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
    }
}

uint64_t TexHook_detoured(tpgxResTexture* tex, void* param_2, void* param_3) {
    if (Logger::IsActive(FileInfo)) Logger::Log(FileInfo) << "Hooked texture: " << std::string(tex->name)
        << " | Original Format: " << XonFormatToString(tex->bxonAssetHeader->XonSurfaceFormat)
        << " | Original Mipmap Count: " << tex->bxonAssetHeader->numMipSurfaces;

    if (Settings::Instance().DumpTextures) {
        dumpTexture(tex);
    }

    std::filesystem::path tex_path(tex->name);
    tex_path.replace_extension(".dds");

    size_t ddsFileSize = 0;
    void* ddsFileData = LoadLooseFile(tex_path.string().c_str(), ddsFileSize);

    if (!ddsFileData) {
        return TexHook_original(tex, param_2, param_3);
    }

    Logger::Log(Info) << "Found loose texture file: " << tex_path.string();

    if (ddsFileSize < sizeof(DDS_HEADER)) {
        Logger::Log(Error) << " | Error: DDS file is too small to be valid.";
        return TexHook_original(tex, param_2, param_3);
    }
    DDS_HEADER* ddsHeader = static_cast<DDS_HEADER*>(ddsFileData);
    if (ddsHeader->magic != 0x20534444) {
        Logger::Log(Error) << " | Error: Invalid DDS magic.";
        return TexHook_original(tex, param_2, param_3);
    }

    XonSurfaceDXGIFormat finalFormat = UNKNOWN;
    void* pixelDataStart = nullptr;
    size_t pixelDataSize = 0;

    if (ddsHeader->ddspf_dwFourCC == '01XD') {
        Logger::Log(Verbose) << " | DX10 header detected.";

        DDS_HEADER_DXT10* dx10Header = (DDS_HEADER_DXT10*)((char*)ddsFileData + sizeof(DDS_HEADER));
        finalFormat = DXGIFormatToXonFormat(dx10Header->dxgiFormat);
        pixelDataStart = (char*)dx10Header + sizeof(DDS_HEADER_DXT10);
        pixelDataSize = ddsFileSize - (sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10));
    }
    else {
        Logger::Log(Verbose) << " | Legacy header detected.";
        finalFormat = FourCCToXonFormat(ddsHeader->ddspf_dwFourCC);
        pixelDataStart = (char*)ddsHeader + sizeof(DDS_HEADER);
        pixelDataSize = ddsFileSize - sizeof(DDS_HEADER);
    }

    if (finalFormat == UNKNOWN) {
        Logger::Log(Error) << " | Unsupported DDS pixel format in loose file.";
        return TexHook_original(tex, param_2, param_3);
    }

    if (ddsHeader->dwMipMapCount != tex->bxonAssetHeader->numMipSurfaces) {
        Logger::Log(Error) << " | Loose DDS has different mipmap count (" << ddsHeader->dwMipMapCount
            << ") than original texture (" << tex->bxonAssetHeader->numMipSurfaces << ").";
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
        uint32_t mipWidth = std::max(1u, currentWidth);
        uint32_t mipHeight = std::max(1u, currentHeight);
        uint32_t mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * bytesPerBlock;

        pMipSurfaces[i].offset = currentOffset;
        pMipSurfaces[i].size = mipSize;
        pMipSurfaces[i].width = mipWidth;
        pMipSurfaces[i].height = mipHeight;

        currentOffset += mipSize;
        if (currentWidth > 1) currentWidth /= 2;
        if (currentHeight > 1) currentHeight /= 2;
    }


    return TexHook_original(tex, param_2, param_3);
}


bool InstallTextureHooks() {
    if (MH_CreateHookEx(TexHook_target, &TexHook_detoured, &TexHook_original) != MH_OK) {
        Logger::Log(Error) << "Could not create texture hook";
        return false;
    }
    return true;
}