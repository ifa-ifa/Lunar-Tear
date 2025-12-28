#pragma once
#include "replicant/core/reader.h"
#include <vector>
#include <cstdint>
#include <expected>
#include <span>

namespace replicant {

    enum class TextureFormat : uint8_t {
        R32G32B32A32_FLOAT = 0,
        R32G32B32_FLOAT = 1,
        R32G32_FLOAT = 2,
        R32_FLOAT = 3,
        R16G16B16A16_FLOAT = 4,
        R16G16_FLOAT = 5,
        R16_FLOAT = 6,
        R8G8B8A8_UNORM = 7,
        R8G8B8A8_UNORM_SRGB = 8,
        R8G8_UNORM = 9,
        R8_UNORM = 10,
        B8G8R8A8_UNORM = 11,
        B8G8R8A8_UNORM_SRGB = 12,
        B8G8R8X8_UNORM = 13,
        B8G8R8X8_UNORM_SRGB = 14,
        BC1_UNORM = 15,
        BC1_UNORM_SRGB = 16,
        BC2_UNORM = 17,
        BC2_UNORM_SRGB = 18,
        BC3_UNORM = 19,
        BC3_UNORM_SRGB = 20,
        BC4_UNORM = 21,
        BC5_UNORM = 22,
        BC6H_UF16 = 23,
        BC6H_SF16 = 24,
        BC7_UNORM = 25,
        BC7_UNORM_SRGB = 26,
        UNKNOWN = 27
    };

    enum class TextureDimension : uint8_t {
        Texture2D = 1,
        Texture3D = 2,
        CubeMap = 3
    };

    struct MipSurface {
        uint32_t offset;    // Offset relative to the start of pixel data
        uint32_t rowPitch;
        uint32_t sliceSize;
        uint32_t width;
        uint32_t height;
        uint32_t depth;    
        uint32_t rowCount;
    };

    struct TextureHeader {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 0; 
        uint32_t mipCount = 0;
        uint32_t totalPixelSize = 0;

        TextureFormat format = TextureFormat::UNKNOWN;
        TextureDimension dimension = TextureDimension::Texture2D;

        std::vector<MipSurface> mips;

        uint32_t calculateArraySize() const {
            if (mipCount == 0) return 0;
            return static_cast<uint32_t>(mips.size()) / mipCount;
        }
    };


    class Texture {
    public:
        static std::expected<TextureHeader, ReaderError> DeserializeHeader(std::span<const std::byte> data);
        static std::vector<std::byte> SerializeHeader(const TextureHeader& header);
    };
}

namespace replicant::raw {

#pragma pack(push, 1)
    struct RawSurfaceFormat {
        uint8_t usageMaybe;
        TextureFormat resourceFormat;
        TextureDimension resourceDimension;
        uint8_t generateMips;
    };

    struct RawTexHeader {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mipCount;
        uint32_t size;
        uint32_t unknownOffset;
        RawSurfaceFormat format;
        uint32_t subresourcesCount;
        uint32_t offsetToSubresources;
    };

    struct RawMipSurface {
        uint32_t offset;
        uint32_t unknown0;
        uint32_t rowPitch;
        uint32_t unknown1;
        uint32_t sliceSize;
        uint32_t unknown2;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t rowCount;
    };
#pragma pack(pop)
}
