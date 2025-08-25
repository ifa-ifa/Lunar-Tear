#include "replicant/bxon/texture_builder.h"
#include "replicant/bxon/texture.h"
#include <vector>
#include <string>
#include <numeric>
#include <cstring>
#include <algorithm>
#include <optional>

namespace {

    enum DXGI_FORMAT {
        DXGI_FORMAT_UNKNOWN = 0,
        DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
        DXGI_FORMAT_R8G8B8A8_UNORM = 28,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
        DXGI_FORMAT_A8_UNORM = 65,
        DXGI_FORMAT_BC1_UNORM = 71,
        DXGI_FORMAT_BC1_UNORM_SRGB = 72,
        DXGI_FORMAT_BC2_UNORM = 74,
        DXGI_FORMAT_BC2_UNORM_SRGB = 75,
        DXGI_FORMAT_BC3_UNORM = 77,
        DXGI_FORMAT_BC3_UNORM_SRGB = 78,
        DXGI_FORMAT_BC4_UNORM = 80,
        DXGI_FORMAT_BC5_UNORM = 83,
        DXGI_FORMAT_BC7_UNORM = 98,
        DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    };

    size_t align_to(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

#pragma pack(push, 1)
    struct RawBxonHeader { char magic[4]; uint32_t version; uint32_t projectID; uint32_t offsetToAssetTypeName; uint32_t offsetToAssetTypeData; };
    struct RawGxTexHeader {
        uint32_t texWidth;
        uint32_t texHeight;
        uint32_t surfaces;
        uint32_t mipmapLevels;
        uint32_t texSize;
        uint32_t unknownInt1;
        replicant::bxon::XonSurfaceDXGIFormat XonSurfaceFormat;
        uint32_t numMipSurfaces;
        uint32_t offsetToMipSurface;
    };
    struct RawGxTexMipSurface {
        uint32_t offset;
        uint32_t unknown_1;
        uint32_t rowPitch_bpr;
        uint32_t unknown_3;
        uint32_t size;
        uint32_t unknown_5;
        uint32_t width;
        uint32_t height;
        uint32_t depthSliceCount;
        uint32_t rowCount;
    };

    struct _DDS_PIXELFORMAT {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask;
        uint32_t dwGBitMask;
        uint32_t dwBBitMask;
        uint32_t dwABitMask;
    };

    struct _DDS_HEADER {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwHeight;
        uint32_t dwWidth;
        uint32_t dwPitchOrLinearSize;
        uint32_t dwDepth;
        uint32_t dwMipMapCount;
        uint32_t dwReserved1[11];
        _DDS_PIXELFORMAT ddspf;
        uint32_t dwCaps;
        uint32_t dwCaps2;
        uint32_t dwCaps3;
        uint32_t dwCaps4;
        uint32_t dwReserved2;
    };

    struct DDS_HEADER_DXT10 {
        DXGI_FORMAT dxgiFormat;
        uint32_t    resourceDimension;
        uint32_t    miscFlag;
        uint32_t    arraySize;
        uint32_t    miscFlags2;
    };
#pragma pack(pop)

    std::optional<replicant::bxon::XonSurfaceDXGIFormat> ddsFormatToXonFormat(const _DDS_HEADER* dds_header) {
        const _DDS_PIXELFORMAT& pf = dds_header->ddspf;

        if (pf.dwFourCC == '01XD') { // "DX10"
            const auto* dx10_header = reinterpret_cast<const DDS_HEADER_DXT10*>(dds_header + 1);
            switch (dx10_header->dxgiFormat) {
            case DXGI_FORMAT_R8G8B8A8_UNORM:       return replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM_SRGB;
            case DXGI_FORMAT_BC1_UNORM:           return replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM;
            case DXGI_FORMAT_BC1_UNORM_SRGB:      return replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM_SRGB;
            case DXGI_FORMAT_BC2_UNORM:           return replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM;
            case DXGI_FORMAT_BC2_UNORM_SRGB:      return replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM_SRGB;
            case DXGI_FORMAT_BC3_UNORM:           return replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM;
            case DXGI_FORMAT_BC3_UNORM_SRGB:      return replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM_SRGB;
            case DXGI_FORMAT_BC4_UNORM:           return replicant::bxon::XonSurfaceDXGIFormat::BC4_UNORM;
            case DXGI_FORMAT_BC5_UNORM:           return replicant::bxon::XonSurfaceDXGIFormat::BC5_UNORM;
            case DXGI_FORMAT_BC7_UNORM:           return replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM;
            case DXGI_FORMAT_BC7_UNORM_SRGB:      return replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM;
            case DXGI_FORMAT_A8_UNORM:            return replicant::bxon::XonSurfaceDXGIFormat::UNKN_A8_UNORM;
            default:                              return std::nullopt;
            }
        }

        // legacy format
        switch (pf.dwFourCC) {
        case '1TXD': return replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM;
        case '3TXD': return replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM;
        case '5TXD': return replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM;
        case '1ITA': return replicant::bxon::XonSurfaceDXGIFormat::BC4_UNORM;
        case '2ITA': case 'U5CB': case 'S5CB': case '5CB\0':
            return replicant::bxon::XonSurfaceDXGIFormat::BC5_UNORM;
        }

        //  check flags for uncompressed formats
        if ((pf.dwFlags & 0x40) || ((pf.dwFlags & 0x1) && pf.dwRGBBitCount > 0)) { // DDPF_RGB or DDPF_ALPHA
            if (pf.dwRGBBitCount == 32 && pf.dwRBitMask == 0x000000FF && pf.dwGBitMask == 0x0000FF00 && pf.dwBBitMask == 0x00FF0000 && pf.dwABitMask == 0xFF000000) {
                return replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM;
            }
            if (pf.dwRGBBitCount == 8 && pf.dwABitMask == 0xFF) {
                return replicant::bxon::XonSurfaceDXGIFormat::UNKN_A8_UNORM;
            }
        }

        return std::nullopt;
    }

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
            // 8 bytes per 4x4 block
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
        { // Add SRGB variant here ????????????????????

            // 16 bytes per 4x4 block
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

    std::expected<std::vector<char>, BxonError> buildTextureFromDDS(
        const std::vector<char>& dds_data,
        uint32_t version,
        uint32_t projectID)
    {
        if (dds_data.size() < sizeof(_DDS_HEADER) + 4 || memcmp(dds_data.data(), "DDS ", 4) != 0) {
            return std::unexpected(BxonError{ BxonErrorCode::BuildError, "Invalid DDS file magic or size." });
        }

        const auto* dds_header = reinterpret_cast<const _DDS_HEADER*>(dds_data.data() + 4);

        const char* texture_data_start;
        bool has_dx10_header = (dds_header->ddspf.dwFourCC == '01XD');
        if (has_dx10_header) {
            texture_data_start = dds_data.data() + 4 + sizeof(_DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
        }
        else {
            texture_data_start = dds_data.data() + 4 + sizeof(_DDS_HEADER);
        }

        auto xon_format_opt = ddsFormatToXonFormat(dds_header);
        if (!xon_format_opt) {
            std::string error_msg = "Unsupported DDS pixel format. ";
            const auto& pf = dds_header->ddspf;
            if (pf.dwFourCC == '01XD') {
                const auto* dx10_header = reinterpret_cast<const DDS_HEADER_DXT10*>(dds_header + 1);
                error_msg += "DX10 Format ID: " + std::to_string(dx10_header->dxgiFormat);
            }
            else if (pf.dwFlags & 0x4) {
                char fourcc_str[5] = { 0 };
                memcpy(fourcc_str, &pf.dwFourCC, 4);
                error_msg += "FourCC: " + std::string(fourcc_str);
            }
            else {
                error_msg += "Uncompressed Format Details: BitCount=" + std::to_string(pf.dwRGBBitCount)
                    + ", RMask=" + std::to_string(pf.dwRBitMask)
                    + ", GMask=" + std::to_string(pf.dwGBitMask)
                    + ", BMask=" + std::to_string(pf.dwBBitMask)
                    + ", AMask=" + std::to_string(pf.dwABitMask);
            }
            return std::unexpected(BxonError{ BxonErrorCode::BuildError, error_msg });
        }
        XonSurfaceDXGIFormat xon_format = *xon_format_opt;

        const uint32_t mip_levels = (dds_header->dwMipMapCount > 0) ? dds_header->dwMipMapCount : 1;
        const std::string asset_type_name = "tpGxTexHead";
        constexpr size_t ALIGNMENT = 16;

        const uint32_t header_start = 0;
        const uint32_t asset_type_name_start = align_to(header_start + sizeof(RawBxonHeader), ALIGNMENT);
        const uint32_t asset_param_start = 8 + align_to(asset_type_name_start + asset_type_name.length() + 1, 8);

        // The mip surface array must be aligned 16 bytes from the start of tpGxTexHead header
        // asset_param_start is where tpGxTexHead header begins
        const uint32_t tex_header_end = asset_param_start + sizeof(RawGxTexHeader);

        // Find the next 16-byte aligned position relative to the tpGxTexHead header start
        const uint32_t distance_from_header_start = tex_header_end - asset_param_start;
        const uint32_t next_16_aligned_distance = align_to(distance_from_header_start, 16);
        const uint32_t mip_surface_array_start = asset_param_start + next_16_aligned_distance;

        const uint32_t texture_data_block_start = mip_surface_array_start + (sizeof(RawGxTexMipSurface) * mip_levels);

        size_t total_texture_data_size = 0;
        uint32_t temp_width = dds_header->dwWidth;
        uint32_t temp_height = dds_header->dwHeight;
        for (uint32_t i = 0; i < mip_levels; ++i) {
            total_texture_data_size += calculateMipSize(temp_width, temp_height, xon_format);
            temp_width = std::max(1u, temp_width / 2);
            temp_height = std::max(1u, temp_height / 2);
        }

        const size_t total_size = texture_data_block_start + total_texture_data_size;
        std::vector<char> buffer(total_size, 0);

        auto* header = reinterpret_cast<RawBxonHeader*>(buffer.data() + header_start);
        memcpy(header->magic, "BXON", 4);
        header->version = version;
        header->projectID = projectID;
        header->offsetToAssetTypeName = asset_type_name_start - (header_start + offsetof(RawBxonHeader, offsetToAssetTypeName));
        header->offsetToAssetTypeData = asset_param_start - (header_start + offsetof(RawBxonHeader, offsetToAssetTypeData));

        memcpy(buffer.data() + asset_type_name_start, asset_type_name.c_str(), asset_type_name.length() + 1);

        auto* tex_header = reinterpret_cast<RawGxTexHeader*>(buffer.data() + asset_param_start);
        tex_header->texWidth = dds_header->dwWidth;
        tex_header->texHeight = dds_header->dwHeight;
        tex_header->surfaces = 1;
        tex_header->mipmapLevels = mip_levels;
        tex_header->texSize = total_texture_data_size;
        tex_header->unknownInt1 = 2147483648;
        tex_header->XonSurfaceFormat = xon_format;
        tex_header->numMipSurfaces = mip_levels;
        // Set the offset to the mip surface array relative to the tex header field
        tex_header->offsetToMipSurface = mip_surface_array_start - (asset_param_start + offsetof(RawGxTexHeader, offsetToMipSurface));

        auto* mip_surfaces = reinterpret_cast<RawGxTexMipSurface*>(buffer.data() + mip_surface_array_start);
        uint32_t current_tex_data_offset_in_dds = 0;
        uint32_t current_tex_data_offset_in_bxon = 0;
        uint32_t current_width = dds_header->dwWidth;
        uint32_t current_height = dds_header->dwHeight;

        for (uint32_t i = 0; i < mip_levels; ++i) {
            size_t mip_size = calculateMipSize(current_width, current_height, xon_format);

            uint32_t pitch = 0;
            uint32_t rows = 0;
            calculateMipPitchAndRows(current_width, current_height, xon_format, pitch, rows);

            mip_surfaces[i].rowPitch_bpr = pitch;
            mip_surfaces[i].rowCount = rows;
            mip_surfaces[i].offset = current_tex_data_offset_in_bxon;
            mip_surfaces[i].size = mip_size;
            mip_surfaces[i].width = current_width;
            mip_surfaces[i].height = current_height;
            mip_surfaces[i].depthSliceCount = 1;
            mip_surfaces[i].unknown_1 = 0;
            mip_surfaces[i].unknown_3 = 0;
            mip_surfaces[i].unknown_5 = 0;

            memcpy(buffer.data() + texture_data_block_start + current_tex_data_offset_in_bxon,
                texture_data_start + current_tex_data_offset_in_dds,
                mip_size);

            current_tex_data_offset_in_dds += mip_size;
            current_tex_data_offset_in_bxon += mip_size;
            current_width = std::max(1u, current_width / 2);
            current_height = std::max(1u, current_height / 2);
        }

        return buffer;
    }
}