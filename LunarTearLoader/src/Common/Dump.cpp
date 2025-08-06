#define NOMINMAX
#include "dump.h"
#include <fstream>
#include <vector>
#include "Game/TextureFormats.h"
#include <filesystem>
#include <settbll.h>
#include "Common/Logger.h"

constexpr size_t MAX_STBL_SIZE = 16 * 1000 * 1000;

namespace {
    constexpr uint32_t DDS_MAGIC = 0x20534444; // 'DDS '
    constexpr uint32_t DDPF_FOURCC = 0x4;
    constexpr uint32_t DDSD_CAPS = 0x1;
    constexpr uint32_t DDSD_HEIGHT = 0x2;
    constexpr uint32_t DDSD_WIDTH = 0x4;
    constexpr uint32_t DDSD_PITCH = 0x8;
    constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
    constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;
    constexpr uint32_t DDSD_LINEARSIZE = 0x80000;
    constexpr uint32_t DDSCAPS_COMPLEX = 0x8;
    constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;
    constexpr uint32_t DDSCAPS_MIPMAP = 0x400000;

    uint32_t GetLegacyFourCC(XonSurfaceDXGIFormat format) {
        switch (format) {
        case BC1_UNORM: return '1TXD'; // DXT1
        case BC2_UNORM: return '3TXD'; // DXT3
        case BC3_UNORM: return '5TXD'; // DXT5
        case BC4_UNORM: return '1ITA'; // ATI1
        case BC5_UNORM: return '2ITA'; // ATI2
        default: return 0;
        }
    }
}

using enum Logger::LogCategory;

void dumpScript(const std::string& name, const std::vector<char>& data) {
	if (!std::filesystem::exists("LunarTear/dump/scripts/")) {
		std::filesystem::create_directories("LunarTear/dump/scripts/");
	}
	std::ofstream f("LunarTear/dump/scripts/" + name, std::ios::binary);
	f.write(data.data(), data.size());

}

void dumpTexture(const tpgxResTexture* tex) {
    if (!tex || !tex->name || !tex->bxonAssetHeader || !tex->texData) {
        Logger::Log(Error) << "Attempted to dump an invalid texture";
        return;
    }

    std::filesystem::path out_path("LunarTear/dump/textures/");
    std::filesystem::path tex_filename(tex->name);
    out_path /= tex_filename.replace_extension(".dds");

    if (!std::filesystem::exists(out_path.parent_path())) {
        std::filesystem::create_directories(out_path.parent_path());
    }

    DDS_HEADER header{};
    header.magic = DDS_MAGIC;
    header.dwSize = sizeof(DDS_HEADER);
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    header.dwHeight = tex->bxonAssetHeader->texHeight;
    header.dwWidth = tex->bxonAssetHeader->texWidth;
    header.dwMipMapCount = tex->bxonAssetHeader->numMipSurfaces > 0 ? tex->bxonAssetHeader->numMipSurfaces : 1;
    if (header.dwMipMapCount > 1) {
        header.dwFlags |= DDSD_MIPMAPCOUNT;
    }
    header.ddspf_dwSize = 32; // sizeof(DDS_PIXELFORMAT) 
    header.dwCaps = DDSCAPS_TEXTURE;
    if (header.dwMipMapCount > 1) {
        header.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    }

    XonSurfaceDXGIFormat xonFormat = tex->bxonAssetHeader->XonSurfaceFormat;
    uint32_t legacyFourCC = GetLegacyFourCC(xonFormat);
    bool useDx10Header = (legacyFourCC == 0);

    bool isBlockCompressed = false;
    uint32_t pitchOrLinearSize = 0;

    switch (xonFormat) {
    case BC1_UNORM:
    case BC1_UNORM_SRGB:
    case BC4_UNORM:
        isBlockCompressed = true;
        pitchOrLinearSize = std::max(1u, (header.dwWidth + 3) / 4) * std::max(1u, (header.dwHeight + 3) / 4) * 8;
        break;
    case BC2_UNORM:
    case BC2_UNORM_SRGB:
    case BC3_UNORM:
    case BC3_UNORM_SRGB:
    case BC5_UNORM:
    case BC7_UNORM:
        isBlockCompressed = true;
        pitchOrLinearSize = std::max(1u, (header.dwWidth + 3) / 4) * std::max(1u, (header.dwHeight + 3) / 4) * 16;
        break;
    case R8G8B8A8_UNORM_STRAIGHT:
    case R8G8B8A8_UNORM:
    case R8G8B8A8_UNORM_SRGB:
        pitchOrLinearSize = header.dwWidth * 4; // 32 bpp
        break;
    case R32G32B32A32_FLOAT:
        pitchOrLinearSize = header.dwWidth * 16; // 128 bpp
        break;
    case UNKN_A8_UNORM:
        pitchOrLinearSize = header.dwWidth * 1; // 8 bpp
        break;
    default:
        Logger::Log(Error) << "Cannot dump texture '" << tex->name << "': unsupported format " << xonFormat << " (" << XonFormatToString(xonFormat) << ")";
        return;
    }

    if (isBlockCompressed) {
        header.dwFlags |= DDSD_LINEARSIZE;
        header.dwPitchOrLinearSize = pitchOrLinearSize;
    }
    else {
        header.dwFlags |= DDSD_PITCH;
        header.dwPitchOrLinearSize = pitchOrLinearSize;
    }

    DDS_HEADER_DXT10 dx10Header{};
    if (useDx10Header) {
        header.ddspf_dwFlags = DDPF_FOURCC;
        header.ddspf_dwFourCC = '01XD'; // "DX10"

        dx10Header.dxgiFormat = XonFormatToDxgiFormat(xonFormat);
        if (dx10Header.dxgiFormat == DXGI_FORMAT_UNKNOWN) {
            Logger::Log(Error) << "Cannot dump texture '" << tex->name << "': failed to map XON format " << xonFormat << " (" << XonFormatToString(xonFormat) << ") to DXGI format.";
            return;
        }
        dx10Header.resourceDimension = 3; // D3D10_RESOURCE_DIMENSION_TEXTURE2D
        dx10Header.miscFlag = 0;
        dx10Header.arraySize = 1;
        dx10Header.miscFlags2 = 0;
    }
    else {
        header.ddspf_dwFlags = DDPF_FOURCC;
        header.ddspf_dwFourCC = legacyFourCC;
    }

    std::ofstream f(out_path, std::ios::binary);
    if (!f.is_open()) {
        Logger::Log(Error) << "Failed to open file for texture dump: " << out_path.string();
        return;
    }

    f.write(reinterpret_cast<const char*>(&header), sizeof(DDS_HEADER));
    if (useDx10Header) {
        f.write(reinterpret_cast<const char*>(&dx10Header), sizeof(DDS_HEADER_DXT10));
    }

    f.write(static_cast<const char*>(tex->texData), tex->texDataSize);

    Logger::Log(Info) << "Dumped texture '" << tex->name << "' to '" << out_path.string() << "'";
}


void dumpTable(const std::string& name, const char* data) {
    size_t inferred_size = StblFile::InferSize(data, MAX_STBL_SIZE);

    if (inferred_size == 0) {
        Logger::Log(Error) << "Failed to infer size for table dump: " << name;
        return;
    }

    if (!std::filesystem::exists("LunarTear/dump/tables/")) {
        std::filesystem::create_directories("LunarTear/dump/tables/");
    }

    std::ofstream f("LunarTear/dump/tables/" + name, std::ios::binary);
    
    f.write(data, inferred_size);
    Logger::Log(Info) << "Dumped table '" << name << "' (" << inferred_size << " bytes)";
    

}