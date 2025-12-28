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


using enum Logger::LogCategory;

namespace {
    typedef uint64_t(__fastcall* TexHook_t)(tpGxResTexture*, void*, void*);
    TexHook_t TexHook_original = nullptr;

    replicant::TextureFormat ToLinear(replicant::TextureFormat f) {
        using enum replicant::TextureFormat;
        switch (f) {
        case R8G8B8A8_UNORM_SRGB: return R8G8B8A8_UNORM;
        case B8G8R8A8_UNORM_SRGB: return B8G8R8A8_UNORM;
        case B8G8R8X8_UNORM_SRGB: return B8G8R8X8_UNORM;
        case BC1_UNORM_SRGB: return BC1_UNORM;
        case BC2_UNORM_SRGB: return BC2_UNORM;
        case BC3_UNORM_SRGB: return BC3_UNORM;
        case BC7_UNORM_SRGB: return BC7_UNORM;
        default: return f;
        }
    }

}

uint64_t TexHook_detoured(tpGxResTexture* tex, void* param_2, void* param_3) {

    if (Logger::IsActive(FileInfo)) Logger::Log(FileInfo) << "Hooked texture: " << std::string(tex->name)
        << " | Original Format: " << XonFormatToString(tex->bxonAssetHeader->format)
        << " | Original Mipmap Count: " << tex->bxonAssetHeader->mipCount;

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

    // If not found, try loading by CRC32c hash (older mods based on SpecialK injection use this)
    if (!ddsFileData) {
        uintptr_t offsetFieldAddr = reinterpret_cast<uintptr_t>(&tex->bxonAssetHeader->offsetToSubresources);
        uintptr_t mipTableAddr = offsetFieldAddr + tex->bxonAssetHeader->offsetToSubresources;

        auto* pMipSurfaces = reinterpret_cast<replicant::raw::RawMipSurface*>(mipTableAddr);

        if (tex->bxonAssetHeader->mipCount > 0) {
            size_t lod0_size = pMipSurfaces[0].sliceSize;

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

    std::span<const std::byte> ddsSpan(reinterpret_cast<const std::byte*>(ddsFileData), ddsFileSize);
    auto dds_result = replicant::dds::DDSFile::LoadFromMemory(ddsSpan);
    if (!dds_result) {
        Logger::Log(Error) << " | Failed to parse loose DDS file with libreplicant: " << dds_result.error().message;
        return TexHook_original(tex, param_2, param_3);
	}

    const auto& dds_file = *dds_result;

    auto raw_original_format = (replicant::raw::RawSurfaceFormat)tex->bxonAssetHeader->format;
    auto raw_mod_format = dds_file.getFormat();


    bool format_ok = (raw_original_format.resourceFormat == raw_mod_format);

    if (!format_ok && Settings::Instance().AllowColourSpaceMismatch) {
        if (ToLinear(raw_mod_format) == ToLinear(raw_original_format.resourceFormat)) {
            Logger::Log(Warning) << " | Allowing sRGB/non-sRGB format mismatch for compatibility.";
            format_ok = true;
        }
    }

    // This form of texture replacment requires identical texture size and mipmap layout. Check this and do not load a texture that does not match.

    if (!format_ok) {
        Logger::Log(Error) << " | Format mismatch. Original is " << XonFormatToString(raw_original_format)
            << ", but mod is " << TextureFormatToString(raw_mod_format) << ". Texture not replaced. Note that game specific flags like volumetric are not relevant here.";
        return TexHook_original(tex, param_2, param_3);
    }

    if (tex->bxonAssetHeader->width != dds_file.getWidth() || tex->bxonAssetHeader->width != dds_file.getHeight()) {
        Logger::Log(Error) << " | Resolution mismatch. Original is " << tex->bxonAssetHeader->width << "x" << tex->bxonAssetHeader->height
            << ", but mod is " << dds_file.getWidth() << "x" << dds_file.getHeight() << ". Texture not replaced.";
        return TexHook_original(tex, param_2, param_3);
    }
    if (tex->bxonAssetHeader->depth != dds_file.getDepth()) {
        Logger::Log(Error) << " | Depth mismatch (Volumetric). Original is " << tex->bxonAssetHeader->depth
            << ", but mod is " << dds_file.getDepth() << ". Texture not replaced.";
        return TexHook_original(tex, param_2, param_3);
    }
    if (tex->bxonAssetHeader->mipCount != dds_file.getMipLevels()) {
        Logger::Log(Error) << " | Mipmap count mismatch. Original has " << tex->bxonAssetHeader->mipCount
            << " mips, but mod has " << dds_file.getMipLevels() << ". Texture not replaced.";
        return TexHook_original(tex, param_2, param_3);
    }


    size_t headerSize = 128;
    const uint32_t* raw_uints = static_cast<const uint32_t*>(ddsFileData);
    if (raw_uints[21] == 0x30315844) { // "DX10"
        headerSize = 148;
    }
    

    tex->texData = static_cast<char*>(ddsFileData) + headerSize;
    tex->texDataSize = ddsFileSize - headerSize;
    tex->bxonAssetHeader->size = tex->texDataSize;

    Logger::Log(Verbose) << " | Successfully replaced texture data for " << tex->name;

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