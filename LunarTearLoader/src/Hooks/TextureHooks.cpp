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

using enum Logger::LogCategory;

namespace {
    typedef uint64_t(__fastcall* TexHook_t)(tpgxResTexture*, void*, void*);
    TexHook_t TexHook_original = nullptr;

}

uint64_t TexHook_detoured(tpgxResTexture* tex, void* param_2, void* param_3) {
    if (Logger::IsActive(FileInfo)) Logger::Log(FileInfo) << "Hooked texture: " << std::string(tex->name)
        << " | Original Format: " << XonFormatToString(tex->bxonAssetHeader->XonSurfaceFormat)
        << " | Original Mipmap Count: " << tex->bxonAssetHeader->numMipSurfaces;

    if (Settings::Instance().DumpTextures) {
        dumpTexture(tex);
    }

    // Try loading by original filename
    std::filesystem::path tex_path(tex->name);
    tex_path.replace_extension(".dds");

    size_t ddsFileSize = 0;
    void* ddsFileData = LoadLooseFile(tex_path.string().c_str(), ddsFileSize);

    bool foundByHash = false;
    std::string hashed_filename;

    // If not found, try loading by CRC32c hash (older mods based on SpecialK's injection use this)
    if (!ddsFileData) {
        mipSurface* pMipSurfaces = (mipSurface*)((uintptr_t)&tex->bxonAssetHeader->offsetToMipSurfaces + tex->bxonAssetHeader->offsetToMipSurfaces);
        if (tex->bxonAssetHeader->numMipSurfaces > 0 && pMipSurfaces) {
            size_t lod0_size = pMipSurfaces[0].size;

            uint32_t hash = crc32c::Crc32c((char*)tex->texData, lod0_size);


            std::stringstream ss;
            ss << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << hash << ".dds";
            hashed_filename = ss.str();
            Logger::Log(Verbose) << "Could not find texture by name. Trying SpecialK hash: " << hashed_filename;

            ddsFileData = LoadLooseFile(hashed_filename.c_str(), ddsFileSize);
            if (ddsFileData) {
                foundByHash = true;
            }
        }
    }


    if (!ddsFileData) {
        return TexHook_original(tex, param_2, param_3);
    }

    if (foundByHash) {
        Logger::Log(Info) << "Found loose texture file: " << tex_path.string() << " (" << hashed_filename << ")";
    }
    else {
        Logger::Log(Info) << "Found loose texture file: " << tex_path.string();
    }
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

    if (ddsHeader->ddspf_dwFourCC == '01XD') { // "DX10"
        Logger::Log(Verbose) << " | DX10 header detected.";
        DDS_HEADER_DXT10* dx10Header = (DDS_HEADER_DXT10*)((char*)ddsFileData + sizeof(DDS_HEADER));
        finalFormat = DXGIFormatToXonFormat(dx10Header->dxgiFormat);
        pixelDataStart = (char*)dx10Header + sizeof(DDS_HEADER_DXT10);
        pixelDataSize = ddsFileSize - (sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10));
    }
    else if (ddsHeader->ddspf_dwFlags & DDPF_RGB) {
        Logger::Log(Verbose) << " | Uncompressed RGB header detected.";
        // Check for 32bpp BGRA (a very common format)
        if (ddsHeader->ddspf_dwRGBBitCount == 32 &&
            ddsHeader->ddspf_dwRBitMask == 0x00FF0000 &&
            ddsHeader->ddspf_dwGBitMask == 0x0000FF00 &&
            ddsHeader->ddspf_dwBBitMask == 0x000000FF)
        {
            finalFormat = R8G8B8A8_UNORM;
        }
        // Check for 32bpp RGBA
        else if (ddsHeader->ddspf_dwRGBBitCount == 32 &&
            ddsHeader->ddspf_dwRBitMask == 0x000000FF &&
            ddsHeader->ddspf_dwGBitMask == 0x0000FF00 &&
            ddsHeader->ddspf_dwBBitMask == 0x00FF0000)
        {
            finalFormat = R8G8B8A8_UNORM;
        }
        pixelDataStart = (char*)ddsHeader + sizeof(DDS_HEADER);
        pixelDataSize = ddsFileSize - sizeof(DDS_HEADER);
    }
    else if (ddsHeader->ddspf_dwFlags & DDPF_FOURCC) {
        Logger::Log(Verbose) << " | Legacy FourCC header detected.";
        finalFormat = FourCCToXonFormat(ddsHeader->ddspf_dwFourCC);
        pixelDataStart = (char*)ddsHeader + sizeof(DDS_HEADER);
        pixelDataSize = ddsFileSize - sizeof(DDS_HEADER);
    }

    if (finalFormat == UNKNOWN) {
        if (ddsHeader->ddspf_dwFlags & DDPF_RGB) {
            Logger::Log(Error) << " | Unsupported DDS pixel format in loose file. Unrecognized uncompressed (RGB) format with bitcount: " << ddsHeader->ddspf_dwRGBBitCount;
        }
        else {
            char fourCCStr[5] = { 0 };
            memcpy(fourCCStr, &ddsHeader->ddspf_dwFourCC, 4);
            Logger::Log(Error) << " | Unsupported DDS pixel format in loose file - Unrecognized FourCC: '" << fourCCStr << "'";
        }
        return TexHook_original(tex, param_2, param_3);
    }

    if (ddsHeader->dwMipMapCount != tex->bxonAssetHeader->numMipSurfaces) {
        Logger::Log(Error) << " | Loose DDS has different mipmap count (" << ddsHeader->dwMipMapCount
            << ") than original texture (" << tex->bxonAssetHeader->numMipSurfaces << ").";
        return TexHook_original(tex, param_2, param_3);
    }

    if (ddsHeader->dwWidth != tex->bxonAssetHeader->texWidth || ddsHeader->dwHeight != tex->bxonAssetHeader->texHeight) {
        Logger::Log(Error) << " | Loose DDS has different dimensions (" << ddsHeader->dwWidth << "x" << ddsHeader->dwHeight
            << ") than original texture (" << tex->bxonAssetHeader->texWidth << "x" << tex->bxonAssetHeader->texHeight << "). Mismatched resolutions are not supported.";
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

    bool isBlockCompressed = false;
    uint32_t bytesPerPixel = 0;

    switch (finalFormat) {
    case BC1_UNORM:
    case BC1_UNORM_SRGB:
    case BC2_UNORM:
    case BC2_UNORM_SRGB:
    case BC3_UNORM:
    case BC3_UNORM_SRGB:
    case BC4_UNORM:
    case BC5_UNORM:
    case BC7_UNORM:
        isBlockCompressed = true;
        break;
    case R8G8B8A8_UNORM_STRAIGHT:
    case R8G8B8A8_UNORM:
    case R8G8B8A8_UNORM_SRGB:
        bytesPerPixel = 4;
        break;
        // Add other uncompressed formats here if needed
    default:
        Logger::Log(Error) << "Cannot rebuild mip surfaces: Unknown format size for " << XonFormatToString(finalFormat);
        return TexHook_original(tex, param_2, param_3); // Abort if we don't know the size
    }


    for (uint32_t i = 0; i < ddsHeader->dwMipMapCount; i++) {
        uint32_t mipWidth = std::max(1u, currentWidth);
        uint32_t mipHeight = std::max(1u, currentHeight);
        uint32_t mipSize = 0;

        if (isBlockCompressed) {
            bool is_8_byte_format = (finalFormat == BC1_UNORM || finalFormat == BC1_UNORM_SRGB || finalFormat == BC4_UNORM);
            uint32_t bytesPerBlock = is_8_byte_format ? 8 : 16;
            mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * bytesPerBlock;
        }
        else {
            mipSize = mipWidth * mipHeight * bytesPerPixel;
        }

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

    void* TexHook_target = (void*)(g_processBaseAddress + 0x7dc820);

    if (MH_CreateHookEx(TexHook_target, &TexHook_detoured, &TexHook_original) != MH_OK) {
        Logger::Log(Error) << "Could not create texture hook";
        return false;
    }
    return true;
}