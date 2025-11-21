#include "replicant/dds.h"
#include <DirectXTex.h>
#include <fstream>

namespace {
    replicant::bxon::XonSurfaceDXGIFormat DXGIToXon(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:       return replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_BC1_UNORM:            return replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB:       return replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM:            return replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM;
        case DXGI_FORMAT_BC2_UNORM_SRGB:       return replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:            return replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB:       return replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC4_UNORM:            return replicant::bxon::XonSurfaceDXGIFormat::BC4_UNORM;
        case DXGI_FORMAT_BC5_UNORM:            return replicant::bxon::XonSurfaceDXGIFormat::BC5_UNORM;
        case DXGI_FORMAT_BC7_UNORM:            return replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB:       return replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM_SRGB;
        case DXGI_FORMAT_A8_UNORM:             return replicant::bxon::XonSurfaceDXGIFormat::UNKN_A8_UNORM;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:   return replicant::bxon::XonSurfaceDXGIFormat::R32G32B32A32_FLOAT;
        default:                               return replicant::bxon::XonSurfaceDXGIFormat::UNKNOWN;
        }
    }

    DXGI_FORMAT XonToDXGI(replicant::bxon::XonSurfaceDXGIFormat format) {
        switch (format) {
        case replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM:
        case replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM_STRAIGHT:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::R8G8B8A8_UNORM_SRGB:       return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM:                 return DXGI_FORMAT_BC1_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::BC1_UNORM_SRGB:            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM:                 return DXGI_FORMAT_BC2_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::BC2_UNORM_SRGB:            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM:                 return DXGI_FORMAT_BC3_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::BC3_UNORM_SRGB:            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case replicant::bxon::XonSurfaceDXGIFormat::BC4_UNORM:                 return DXGI_FORMAT_BC4_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::BC5_UNORM:                 return DXGI_FORMAT_BC5_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM:                 return DXGI_FORMAT_BC7_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM_SRGB:            return DXGI_FORMAT_BC7_UNORM_SRGB;
        case replicant::bxon::XonSurfaceDXGIFormat::BC7_UNORM_SRGB_VOLUMETRIC: return DXGI_FORMAT_BC7_UNORM_SRGB;
        case replicant::bxon::XonSurfaceDXGIFormat::UNKN_A8_UNORM:             return DXGI_FORMAT_A8_UNORM;
        case replicant::bxon::XonSurfaceDXGIFormat::R32G32B32A32_FLOAT:        return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:                                                               return DXGI_FORMAT_UNKNOWN;
        }
    }
}

namespace replicant::dds {

    DDSFile::DDSFile() : m_image(std::make_unique<DirectX::ScratchImage>()) {}
    DDSFile::~DDSFile() = default;
    DDSFile::DDSFile(DDSFile&& other) noexcept = default;
    DDSFile& DDSFile::operator=(DDSFile&& other) noexcept = default;


    std::expected<DDSFile, DDSError> DDSFile::FromFile(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            return std::unexpected(DDSError{ DDSErrorCode::FileReadError, "File not found: " + path.string() });
        }

        DDSFile ddsFile;
        DirectX::TexMetadata metadata;
        HRESULT hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, *ddsFile.m_image);

        if (FAILED(hr)) {
            return std::unexpected(DDSError{ DDSErrorCode::LibraryError, "DirectXTex failed to load DDS from file." });
        }
        return ddsFile;
    }

    std::expected<DDSFile, DDSError> DDSFile::FromMemory(std::span<const char> data) {
        DDSFile ddsFile;
        DirectX::TexMetadata metadata;
        HRESULT hr = DirectX::LoadFromDDSMemory(
            reinterpret_cast<const std::byte*>(data.data()), data.size(),
            DirectX::DDS_FLAGS_NONE, &metadata, *ddsFile.m_image
        );

        if (FAILED(hr)) {
            return std::unexpected(DDSError{ DDSErrorCode::LibraryError, "DirectXTex failed to load DDS from memory." });
        }
        return ddsFile;
    }

    std::expected<DDSFile, DDSError> DDSFile::FromGameTexture(const bxon::Texture& tex) {
        DXGI_FORMAT dxgi_format = XonToDXGI(tex.getFormat());

        if (dxgi_format == DXGI_FORMAT_UNKNOWN) {

            std::string errstr = "Cannot create DDS from an unsupported game texture format: " + std::to_string(tex.getFormat());
            return std::unexpected(DDSError{ DDSErrorCode::UnsupportedFormat, errstr});
        }

        const auto& all_mip_data = tex.getTextureData();
        const auto& mip_surfaces = tex.getMipSurfaces();
        const size_t mip_levels = tex.getMipSurfaces().size();

        if (mip_surfaces.size() != mip_levels) {
            return std::unexpected(DDSError{ DDSErrorCode::InvalidHeader, "Mismatch between mip level count (" + std::to_string(mip_levels) + ") and mip surface metadata count (" + std::to_string(mip_surfaces.size()) + ")." });
        }

        auto scratch_image = std::make_unique<DirectX::ScratchImage>();
        HRESULT hr;

        if (tex.getDepth() > 1) {
            hr = scratch_image->Initialize3D(
                dxgi_format,
                tex.getWidth(),
                tex.getHeight(),
                tex.getDepth(),
                mip_levels
            );
        }
        else {
            hr = scratch_image->Initialize2D(
                dxgi_format,
                tex.getWidth(),
                tex.getHeight(),
                1,
                mip_levels
            );
        }

        if (FAILED(hr)) {
            return std::unexpected(DDSError{ DDSErrorCode::LibraryError, "DirectXTex failed to initialize a blank ScratchImage for the game texture." });
        }

        for (size_t i = 0; i < mip_levels; ++i) {
            const auto& mip_info = mip_surfaces[i];
            const DirectX::Image* dest_image = scratch_image->GetImage(i, 0, 0);

            if (!dest_image) {
                return std::unexpected(DDSError{ DDSErrorCode::LibraryError, "Could not get destination image pointer for mip level " + std::to_string(i) });
            }

            size_t total_mip_data_size = mip_info.size * mip_info.currentDepth;
            if (mip_info.offset + total_mip_data_size > all_mip_data.size()) {
                return std::unexpected(DDSError{ DDSErrorCode::BufferTooSmall, "Game's mip surface data extends beyond its texture data buffer for mip " + std::to_string(i) });
            }

            if (dest_image->slicePitch != mip_info.size) {
                return std::unexpected(DDSError{ DDSErrorCode::BuildError, "Mismatch between DirectXTex calculated mip size (" + std::to_string(dest_image->slicePitch) + ") and game metadata size (" + std::to_string(mip_info.size) + ") for mip " + std::to_string(i) });
            }

            memcpy(dest_image->pixels, all_mip_data.data() + mip_info.offset, total_mip_data_size);
        }

        DDSFile ddsFile;
        ddsFile.m_image = std::move(scratch_image);
        return ddsFile;
    }


    std::expected<void, DDSError> DDSFile::saveToFile(const std::filesystem::path& path) const {
        if (!m_image || m_image->GetImageCount() == 0) {
            return std::unexpected(DDSError{ DDSErrorCode::BuildError, "Cannot save an empty DDSFile." });
        }

        HRESULT hr = DirectX::SaveToDDSFile(
            m_image->GetImages(), m_image->GetImageCount(), m_image->GetMetadata(),
            DirectX::DDS_FLAGS_NONE, path.c_str()
        );

        if (FAILED(hr)) {
            return std::unexpected(DDSError{ DDSErrorCode::FileWriteError, "DirectXTex failed to save DDS to file." });
        }
        return {};
    }

    std::expected<std::vector<char>, DDSError> DDSFile::saveToMemory() const {
        if (!m_image || m_image->GetImageCount() == 0) {
            return std::unexpected(DDSError{ DDSErrorCode::BuildError, "Cannot save an empty DDSFile." });
        }

        DirectX::Blob blob;
        HRESULT hr = DirectX::SaveToDDSMemory(
            m_image->GetImages(), m_image->GetImageCount(), m_image->GetMetadata(),
            DirectX::DDS_FLAGS_NONE, blob
        );

        if (FAILED(hr)) {
            return std::unexpected(DDSError{ DDSErrorCode::LibraryError, "DirectXTex failed to save DDS to memory blob." });
        }

        const char* buffer_start = reinterpret_cast<char*>(blob.GetBufferPointer());
        return std::vector<char>(buffer_start, buffer_start + blob.GetBufferSize());
    }

    uint32_t DDSFile::getWidth() const noexcept { return m_image ? m_image->GetMetadata().width : 0; }
    uint32_t DDSFile::getHeight() const noexcept { return m_image ? m_image->GetMetadata().height : 0; }
    uint32_t DDSFile::getDepth() const noexcept { return m_image ? m_image->GetMetadata().depth : 0; }

    uint32_t DDSFile::getMipLevels() const noexcept { return m_image ? m_image->GetMetadata().mipLevels : 0; }

    replicant::bxon::XonSurfaceDXGIFormat DDSFile::getFormat() const noexcept {
        return m_image ? DXGIToXon(m_image->GetMetadata().format) : replicant::bxon::XonSurfaceDXGIFormat::UNKNOWN;
    }

    std::span<const char> DDSFile::getPixelData() const noexcept {
        if (!m_image || m_image->GetPixelsSize() == 0) {
            return {};
        }
        return { reinterpret_cast<const char*>(m_image->GetPixels()), m_image->GetPixelsSize() };
    }
}