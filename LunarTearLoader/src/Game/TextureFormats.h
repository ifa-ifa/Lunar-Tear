#pragma once
#include <string>
#include <replicant/tpGxTexHead.h>

#pragma pack(push, 1)
struct tpGxResTexture {
    int64_t unk0[8];
    char* name;
    int64_t unk1[9];
    void* bxonHeader;
    int64_t unk2;
    void* texData;
    int64_t texDataSize;
    int64_t unk3[3];
    replicant::raw::RawTexHeader* bxonAssetHeader;
};
#pragma pack(pop)

inline const char* TextureFormatToString(replicant::TextureFormat format)
{
    switch (format)
    {
    case replicant::TextureFormat::R32G32B32A32_FLOAT: return "R32G32B32A32_FLOAT";
    case replicant::TextureFormat::R32G32B32_FLOAT:    return "R32G32B32_FLOAT";
    case replicant::TextureFormat::R32G32_FLOAT:       return "R32G32_FLOAT";
    case replicant::TextureFormat::R32_FLOAT:          return "R32_FLOAT";
    case replicant::TextureFormat::R16G16B16A16_FLOAT: return "R16G16B16A16_FLOAT";
    case replicant::TextureFormat::R16G16_FLOAT:       return "R16G16_FLOAT";
    case replicant::TextureFormat::R16_FLOAT:          return "R16_FLOAT";
    case replicant::TextureFormat::R8G8B8A8_UNORM:     return "R8G8B8A8_UNORM";
    case replicant::TextureFormat::R8G8B8A8_UNORM_SRGB:return "R8G8B8A8_UNORM_SRGB";
    case replicant::TextureFormat::R8G8_UNORM:         return "R8G8_UNORM";
    case replicant::TextureFormat::R8_UNORM:           return "R8_UNORM";
    case replicant::TextureFormat::B8G8R8A8_UNORM:     return "B8G8R8A8_UNORM";
    case replicant::TextureFormat::B8G8R8A8_UNORM_SRGB:return "B8G8R8A8_UNORM_SRGB";
    case replicant::TextureFormat::B8G8R8X8_UNORM:     return "B8G8R8X8_UNORM";
    case replicant::TextureFormat::B8G8R8X8_UNORM_SRGB:return "B8G8R8X8_UNORM_SRGB";
    case replicant::TextureFormat::BC1_UNORM:          return "BC1_UNORM";
    case replicant::TextureFormat::BC1_UNORM_SRGB:     return "BC1_UNORM_SRGB";
    case replicant::TextureFormat::BC2_UNORM:          return "BC2_UNORM";
    case replicant::TextureFormat::BC2_UNORM_SRGB:     return "BC2_UNORM_SRGB";
    case replicant::TextureFormat::BC3_UNORM:          return "BC3_UNORM";
    case replicant::TextureFormat::BC3_UNORM_SRGB:     return "BC3_UNORM_SRGB";
    case replicant::TextureFormat::BC4_UNORM:          return "BC4_UNORM";
    case replicant::TextureFormat::BC5_UNORM:          return "BC5_UNORM";
    case replicant::TextureFormat::BC6H_UF16:          return "BC6H_UF16";
    case replicant::TextureFormat::BC6H_SF16:          return "BC6H_SF16";
    case replicant::TextureFormat::BC7_UNORM:          return "BC7_UNORM";
    case replicant::TextureFormat::BC7_UNORM_SRGB:     return "BC7_UNORM_SRGB";
    case replicant::TextureFormat::UNKNOWN:            return "UNKNOWN";
    default:                                           return "INVALID_FORMAT";
    }
}

inline const char* TextureDimensionToString(replicant::TextureDimension dim)
{
    switch (dim)
    {
    case replicant::TextureDimension::Texture2D: return "Texture2D";
    case replicant::TextureDimension::Texture3D: return "Texture3D";
    case replicant::TextureDimension::CubeMap:   return "CubeMap";
    default:                                     return "INVALID_DIMENSION";
    }
}

inline const char* XonFormatToString(replicant::raw::RawSurfaceFormat format)
{
    static thread_local std::string result;
    result.clear();

    result += TextureFormatToString(format.resourceFormat);
    result += " | ";
    result += TextureDimensionToString(format.resourceDimension);

    return result.c_str();
}
