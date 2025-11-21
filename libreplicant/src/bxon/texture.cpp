#include "replicant/bxon/texture.h"
#include "replicant/dds.h"
#include <numeric>
#include <cstring>
#include <algorithm>
#include "common.h"

namespace {

    size_t align_to(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

#pragma pack(push, 1)
    struct RawBxonHeader { char magic[4]; uint32_t version; uint32_t projectID; uint32_t offsetToAssetTypeName; uint32_t offsetToAssetTypeData; };
    struct RawGxTexHeader {
        uint32_t texWidth;
        uint32_t texHeight;
        uint32_t texDepth;
        uint32_t numSurfaces;
        uint32_t texSize;
        uint32_t unknownInt1; // always 2147483648 (2**31)
        replicant::bxon::XonSurfaceDXGIFormat XonSurfaceFormat;
        uint32_t numMipSurfaces;
        uint32_t offsetToMipSurface;
    };
#pragma pack(pop)

    size_t calculateMipSize(uint32_t width, uint32_t height, replicant::bxon::XonSurfaceDXGIFormat format) {
        width = std::max(1u, width);
        height = std::max(1u, height);
        switch (format) {
        case replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC4_UNORM:
            return std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 8;
        case replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC5_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM_SRGB_VOLUMETRIC:
            return std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 16;
        case replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM_STRAIGHT:
            return width * height * 4;
        case replicant::bxon::XonSurfaceDXGIFormat::UNKN_A8_UNORM:
            return width * height * 1;
        default:
            return 0;
        }
    }

    void calculateMipPitchAndRows(uint32_t width, uint32_t height, replicant::bxon::XonSurfaceDXGIFormat format,
        uint32_t& out_pitch, uint32_t& out_rows) {
        width = std::max(1u, width);
        height = std::max(1u, height);
        switch (format) {
        case replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC4_UNORM: {
            out_pitch = std::max(1u, (width + 3) / 4) * 8;
            out_rows = std::max(1u, (height + 3) / 4);
            break;
        }
        case replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC5_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM_SRGB:
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM_SRGB_VOLUMETRIC: {
            out_pitch = std::max(1u, (width + 3) / 4) * 16;
            out_rows = std::max(1u, (height + 3) / 4);
            break;
        }
        case replicant::bxon::XonSurfaceDXGIFormat::UNKN_A8_UNORM: {
            out_pitch = width * 1;
            out_rows = height;
            break;
        }
        default:
            out_pitch = 0;
            out_rows = 0;
            break;
        }
    }
}

namespace replicant::bxon {

    bool Texture::parse(const char* buffer, size_t size, const char* asset_base) {
        const auto* header = reinterpret_cast<const RawGxTexHeader*>(asset_base);
        if (reinterpret_cast<const char*>(header + 1) > buffer + size) {
            return false;
        }

        m_width = header->texWidth;
        m_height = header->texHeight;
        m_depth = header->texDepth;
        m_numSurfaces = header->numSurfaces;
        m_totalTextureSize = header->texSize;
        m_format = header->XonSurfaceFormat;

        const char* mip_array_start = reinterpret_cast<const char*>(&header->offsetToMipSurface) + header->offsetToMipSurface;

        if (mip_array_start < buffer || (mip_array_start + header->numMipSurfaces * sizeof(mipSurface)) >(buffer + size)) {
            return false;
        }

        const auto* raw_mips = reinterpret_cast<const mipSurface*>(mip_array_start);
        m_mipSurfaces.assign(raw_mips, raw_mips + header->numMipSurfaces);


        // Calculate where the textures pixel data SHOULD begin in the file buffer
        const char* texture_data_start = mip_array_start + (header->numMipSurfaces * sizeof(mipSurface));

        // We now check if the pixel data actually fits in the provided buffer
        // If it doesnt, we assume we are only parsing a header and return true without reading pixels
        // Probably a shitty solution, need to keep in mind
  
        if ((texture_data_start + m_totalTextureSize) > (buffer + size)) {
    
            return true;
        }

        m_textureData.assign(texture_data_start, texture_data_start + m_totalTextureSize);


        return true;
    }

    std::expected<Texture, BxonError> Texture::FromDDS(const dds::DDSFile& dds_file) {
        Texture tex;
        tex.m_format = dds_file.getFormat();
        if (tex.m_format == XonSurfaceDXGIFormat::UNKNOWN) {
            return std::unexpected(BxonError{ BxonErrorCode::BuildError, "Unsupported or unknown DDS pixel format." });
        }

        tex.m_width = dds_file.getWidth();
        tex.m_height = dds_file.getHeight();
        tex.m_depth = dds_file.getDepth();
        tex.m_numSurfaces = 1;

        auto pixel_data_span = dds_file.getPixelData();
        tex.m_textureData.assign(pixel_data_span.begin(), pixel_data_span.end());
        tex.m_totalTextureSize = tex.m_textureData.size();

        uint32_t current_width = tex.m_width;
        uint32_t current_height = tex.m_height;
        uint32_t current_depth = tex.m_depth;
        uint32_t current_offset = 0;

        const uint32_t mip_levels = dds_file.getMipLevels();
        tex.m_mipSurfaces.reserve(mip_levels);

        for (uint32_t i = 0; i < mip_levels; i++) {
            mipSurface mip;
            mip.size = calculateMipSize(current_width, current_height, tex.m_format);
            calculateMipPitchAndRows(current_width, current_height, tex.m_format, mip.rowPitch, mip.heightInBlocks);

            mip.offset = current_offset;
            mip.width = current_width;
            mip.height = current_height;
            mip.currentDepth = current_depth;
            mip.unk3 = 0;
            mip.unk4 = 0;
            mip.unk5 = 0;

            tex.m_mipSurfaces.push_back(mip);

            current_offset += mip.size;
            current_width = std::max(1u, current_width / 2);
            current_height = std::max(1u, current_height / 2);
            current_depth = std::max(1u, current_depth / 2);
        }

        return tex;
    }

    std::expected<Texture, BxonError> Texture::FromGameData(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t numSurfaces,
        XonSurfaceDXGIFormat format,
        uint32_t totalTextureSize,
        std::span<const mipSurface> mip_surfaces,
        std::span<const char> texture_data)
    {
        if (numSurfaces == 0) {
            return std::unexpected(BxonError{ BxonErrorCode::BuildError, "numSurfaces cannot be zero." });
        }
        if (mip_surfaces.size() % numSurfaces != 0) {
            return std::unexpected(BxonError{ BxonErrorCode::BuildError, "Total number of mip surfaces is not divisible by the number of texture surfaces." });
        }

        Texture tex;
        tex.m_width = width;
        tex.m_height = height;
        tex.m_depth = depth;
        tex.m_numSurfaces = numSurfaces;
        tex.m_format = format;
        tex.m_totalTextureSize = totalTextureSize;

        tex.m_mipSurfaces.assign(mip_surfaces.begin(), mip_surfaces.end());
        tex.m_textureData.assign(texture_data.begin(), texture_data.end());

        return tex;
    }

    std::expected<std::vector<char>, BxonError> Texture::build(uint32_t version, uint32_t projectID) const {
        const std::string asset_type_name = "tpGxTexHead";
        constexpr size_t ALIGNMENT = 16;

        const uint32_t header_start = 0;
        const uint32_t asset_type_name_start = align_to(header_start + sizeof(RawBxonHeader), ALIGNMENT);
        const uint32_t asset_param_start = 8 + align_to(asset_type_name_start + asset_type_name.length() + 1, 8);
        const uint32_t tex_header_end = asset_param_start + sizeof(RawGxTexHeader);
        const uint32_t distance_from_header_start = tex_header_end - asset_param_start;
        const uint32_t next_16_aligned_distance = align_to(distance_from_header_start, 16);
        const uint32_t mip_surface_array_start = asset_param_start + next_16_aligned_distance;
        const uint32_t texture_data_block_start = mip_surface_array_start + (sizeof(mipSurface) * m_mipSurfaces.size());
        const size_t total_size = texture_data_block_start + m_totalTextureSize;

        std::vector<char> buffer(total_size, 0);

        auto* header = reinterpret_cast<RawBxonHeader*>(buffer.data() + header_start);
        memcpy(header->magic, "BXON", 4);
        header->version = version;
        header->projectID = projectID;
        header->offsetToAssetTypeName = asset_type_name_start - (header_start + offsetof(RawBxonHeader, offsetToAssetTypeName));
        header->offsetToAssetTypeData = asset_param_start - (header_start + offsetof(RawBxonHeader, offsetToAssetTypeData));

        memcpy(buffer.data() + asset_type_name_start, asset_type_name.c_str(), asset_type_name.length() + 1);

        auto* tex_header = reinterpret_cast<RawGxTexHeader*>(buffer.data() + asset_param_start);
        tex_header->texWidth = m_width;
        tex_header->texHeight = m_height;
        tex_header->texDepth = m_depth;
        tex_header->numSurfaces = m_numSurfaces;
        tex_header->texSize = m_totalTextureSize;
        tex_header->unknownInt1 = 2147483648;
        tex_header->XonSurfaceFormat = m_format;
        tex_header->numMipSurfaces = m_mipSurfaces.size();
        tex_header->offsetToMipSurface = mip_surface_array_start - (asset_param_start + offsetof(RawGxTexHeader, offsetToMipSurface));

        auto* mip_surfaces_out = reinterpret_cast<mipSurface*>(buffer.data() + mip_surface_array_start);
        for (size_t i = 0; i < m_mipSurfaces.size(); ++i) {
            mip_surfaces_out[i] = m_mipSurfaces[i];
        }

        memcpy(buffer.data() + texture_data_block_start, m_textureData.data(), m_textureData.size());

        return buffer;
    }
}