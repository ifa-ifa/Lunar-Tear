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
#include <replicant/bxon/texture.h>


using enum Logger::LogCategory;

namespace {
    typedef uint64_t(__fastcall* TexHook_t)(tpgxResTexture*, void*, void*);
    TexHook_t TexHook_original = nullptr;

    bool AreFormatsSRGBCompatible(replicant::bxon::XonSurfaceDXGIFormat a, replicant::bxon::XonSurfaceDXGIFormat b) {
        using namespace replicant::bxon;
        if ((a == R8G8B8A8_UNORM && b == R8G8B8A8_UNORM_SRGB) || (a == R8G8B8A8_UNORM_SRGB && b == R8G8B8A8_UNORM)) return true;
        if ((a == BC1_UNORM && b == BC1_UNORM_SRGB) || (a == BC1_UNORM_SRGB && b == BC1_UNORM)) return true;
        if ((a == BC2_UNORM && b == BC2_UNORM_SRGB) || (a == BC2_UNORM_SRGB && b == BC2_UNORM)) return true;
        if ((a == BC3_UNORM && b == BC3_UNORM_SRGB) || (a == BC3_UNORM_SRGB && b == BC3_UNORM)) return true;
        return false;
    }

    // Helper to strip Game Specific flags and return the raw pixel format
    replicant::bxon::XonSurfaceDXGIFormat NormalizeFormat(replicant::bxon::XonSurfaceDXGIFormat fmt) {
        using namespace replicant::bxon;
        if (fmt == BC7_UNORM_SRGB_VOLUMETRIC) {
            return BC7_UNORM_SRGB;
        }
        return fmt;
    }


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

    // If not found, try loading by CRC32c hash (older mods based on SpecialK injection use this)
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


    auto dds_result = replicant::dds::DDSFile::FromMemory({ static_cast<char*>(ddsFileData), ddsFileSize });
    if (!dds_result) {
        Logger::Log(Error) << " | Failed to parse loose DDS file with libreplicant: " << dds_result.error().message;
        return TexHook_original(tex, param_2, param_3);
    }
    const auto& dds_file = *dds_result;

    auto raw_original_format = (replicant::bxon::XonSurfaceDXGIFormat)tex->bxonAssetHeader->XonSurfaceFormat;
    auto raw_mod_format = dds_file.getFormat();

    auto normalized_original = NormalizeFormat(raw_original_format);
    auto normalized_mod = NormalizeFormat(raw_mod_format);

    bool format_ok = (normalized_original == normalized_mod);

    if (!format_ok && Settings::Instance().AllowColourSpaceMismatch) {
        if (AreFormatsSRGBCompatible(normalized_original, normalized_mod)) {
            Logger::Log(Warning) << " | Allowing sRGB/non-sRGB format mismatch for compatibility.";
            format_ok = true;
        }
    }

    // This form of texture replacment requires identical texture size and mipmap layout. Check this and do not load a texture that does not match.

    if (!format_ok) {
        Logger::Log(Error) << " | Format mismatch. Original is " << XonFormatToString(raw_original_format)
            << ", but mod is " << XonFormatToString(raw_mod_format) << ". Texture not replaced. Note that game specific flags like volumetric are not relevant here.";
        return TexHook_original(tex, param_2, param_3);
    }

    if (tex->bxonAssetHeader->texWidth != dds_file.getWidth() || tex->bxonAssetHeader->texHeight != dds_file.getHeight()) {
        Logger::Log(Error) << " | Resolution mismatch. Original is " << tex->bxonAssetHeader->texWidth << "x" << tex->bxonAssetHeader->texHeight
            << ", but mod is " << dds_file.getWidth() << "x" << dds_file.getHeight() << ". Texture not replaced.";
        return TexHook_original(tex, param_2, param_3);
    }
    if (tex->bxonAssetHeader->texDepth != dds_file.getDepth()) {
        Logger::Log(Error) << " | Depth mismatch (Volumetric). Original is " << tex->bxonAssetHeader->texDepth
            << ", but mod is " << dds_file.getDepth() << ". Texture not replaced.";
        return TexHook_original(tex, param_2, param_3);
    }
    if (tex->bxonAssetHeader->numMipSurfaces != dds_file.getMipLevels()) {
        Logger::Log(Error) << " | Mipmap count mismatch. Original has " << tex->bxonAssetHeader->numMipSurfaces
            << " mips, but mod has " << dds_file.getMipLevels() << ". Texture not replaced.";
        return TexHook_original(tex, param_2, param_3);
    }


    auto pixel_data = dds_file.getPixelData();
    tex->texData = (void*)pixel_data.data();
    tex->texDataSize = pixel_data.size();
    tex->bxonAssetHeader->texSize = pixel_data.size();

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