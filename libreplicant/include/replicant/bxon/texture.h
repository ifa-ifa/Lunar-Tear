#pragma once

#include "replicant/bxon/error.h"
#include <cstdint>
#include <vector>
#include <expected>
#include <span>

namespace replicant::dds {
    class DDSFile;
}

namespace replicant::bxon {

    class File;

    enum XonSurfaceDXGIFormat {
        UNKNOWN = 0,
        R8G8B8A8_UNORM_STRAIGHT = 0x00010700,
        R8G8B8A8_UNORM = 0x00010800,
        R8G8B8A8_UNORM_SRGB = 0x00010B00,
        BC1_UNORM = 0x00010F00,
        BC1_UNORM_SRGB = 0x00011000,
        BC2_UNORM = 0x00011100,
        BC2_UNORM_SRGB = 0x00011200,
        BC3_UNORM = 0x00011300,
        BC3_UNORM_SRGB = 0x00011400,
        BC4_UNORM = 0x00011500,
        BC5_UNORM = 0x00011600,
        BC7_UNORM = 0x00011900,
        BC7_UNORM_SRGB = 0x00011A00,
        BC7_UNORM_SRGB_VOLUMETRIC = 0x00021A00,
        R32G32B32A32_FLOAT = 0x00030000,
        UNKN_A8_UNORM = 0x00031700,
    };

    struct mipSurface {
        uint32_t offset; // From texData
        uint32_t unk3; // always 0
        uint32_t rowPitch;
        uint32_t unk4; // always 0
        uint32_t size;
        uint32_t unk5; // always 0
        uint32_t width;
        uint32_t height;
        uint32_t currentDepth;
        uint32_t heightInBlocks;
    };

    /// @brief Represents a 'tpGxTexHead' BXON asset, including its header and pixel data.
    class Texture {
    public:

        static std::expected<Texture, BxonError> FromDDS(const dds::DDSFile& dds_file);
        static std::expected<Texture, BxonError> FromGameData(
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t numSurfaces,
            XonSurfaceDXGIFormat format,
            uint32_t totalTextureSize,
            std::span<const mipSurface> mip_surfaces,
            std::span<const char> texture_data
        );

        std::expected<std::vector<char>, BxonError> build(uint32_t version, uint32_t projectID) const;

        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }
        uint32_t getDepth() const { return m_depth; }
        uint32_t getSurfaceCount() const { return m_numSurfaces; }
        XonSurfaceDXGIFormat getFormat() const { return m_format; }
        const std::vector<mipSurface>& getMipSurfaces() const { return m_mipSurfaces; }
        const std::vector<char>& getTextureData() const { return m_textureData; }
        uint32_t getTotalTextureSize() const { return m_totalTextureSize; }

        friend class File;

    private:

        bool parse(const char* buffer, size_t size, const char* asset_base);


        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_depth = 0;
        uint32_t m_numSurfaces = 0;
        uint32_t m_totalTextureSize = 0;

        XonSurfaceDXGIFormat m_format = XonSurfaceDXGIFormat::UNKNOWN;

        std::vector<mipSurface> m_mipSurfaces;
        std::vector<char> m_textureData;
    };
}