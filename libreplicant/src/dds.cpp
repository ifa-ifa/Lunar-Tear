#define NOMINMAX
#include "replicant/dds.h"
#include <DirectXTex.h>
#include <unordered_map>
#include <algorithm>

namespace {
    using namespace replicant;

    DXGI_FORMAT XonToDXGI(TextureFormat format) {
        switch (format) {
        case TextureFormat::R32G32B32A32_FLOAT:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case TextureFormat::R32G32B32_FLOAT:     return DXGI_FORMAT_R32G32B32_FLOAT;
        case TextureFormat::R32G32_FLOAT:        return DXGI_FORMAT_R32G32_FLOAT;
        case TextureFormat::R32_FLOAT:           return DXGI_FORMAT_R32_FLOAT;
        case TextureFormat::R16G16B16A16_FLOAT:  return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case TextureFormat::R16G16_FLOAT:        return DXGI_FORMAT_R16G16_FLOAT;
        case TextureFormat::R16_FLOAT:           return DXGI_FORMAT_R16_FLOAT;
        case TextureFormat::R8G8B8A8_UNORM:      return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case TextureFormat::R8G8_UNORM:          return DXGI_FORMAT_R8G8_UNORM;
        case TextureFormat::R8_UNORM:            return DXGI_FORMAT_R8_UNORM;
        case TextureFormat::B8G8R8A8_UNORM:      return DXGI_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case TextureFormat::B8G8R8X8_UNORM:      return DXGI_FORMAT_B8G8R8X8_UNORM;
        case TextureFormat::B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case TextureFormat::BC1_UNORM:           return DXGI_FORMAT_BC1_UNORM;
        case TextureFormat::BC1_UNORM_SRGB:      return DXGI_FORMAT_BC1_UNORM_SRGB;
        case TextureFormat::BC2_UNORM:           return DXGI_FORMAT_BC2_UNORM;
        case TextureFormat::BC2_UNORM_SRGB:      return DXGI_FORMAT_BC2_UNORM_SRGB;
        case TextureFormat::BC3_UNORM:           return DXGI_FORMAT_BC3_UNORM;
        case TextureFormat::BC3_UNORM_SRGB:      return DXGI_FORMAT_BC3_UNORM_SRGB;
        case TextureFormat::BC4_UNORM:           return DXGI_FORMAT_BC4_UNORM;
        case TextureFormat::BC5_UNORM:           return DXGI_FORMAT_BC5_UNORM;
        case TextureFormat::BC6H_UF16:           return DXGI_FORMAT_BC6H_UF16;
        case TextureFormat::BC6H_SF16:           return DXGI_FORMAT_BC6H_SF16;
        case TextureFormat::BC7_UNORM:           return DXGI_FORMAT_BC7_UNORM;
        case TextureFormat::BC7_UNORM_SRGB:      return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:                                 return DXGI_FORMAT_UNKNOWN;
        }
    }

    TextureFormat DXGIToXon(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:  return TextureFormat::R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT:     return TextureFormat::R32G32B32_FLOAT;
        case DXGI_FORMAT_R32G32_FLOAT:        return TextureFormat::R32G32_FLOAT;
        case DXGI_FORMAT_R32_FLOAT:           return TextureFormat::R32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:  return TextureFormat::R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R16G16_FLOAT:        return TextureFormat::R16G16_FLOAT;
        case DXGI_FORMAT_R16_FLOAT:           return TextureFormat::R16_FLOAT;
        case DXGI_FORMAT_R8G8B8A8_UNORM:      return TextureFormat::R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return TextureFormat::R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_R8G8_UNORM:          return TextureFormat::R8G8_UNORM;
        case DXGI_FORMAT_R8_UNORM:            return TextureFormat::R8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM:      return TextureFormat::B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return TextureFormat::B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8X8_UNORM:      return TextureFormat::B8G8R8X8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return TextureFormat::B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_BC1_UNORM:           return TextureFormat::BC1_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB:      return TextureFormat::BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM:           return TextureFormat::BC2_UNORM;
        case DXGI_FORMAT_BC2_UNORM_SRGB:      return TextureFormat::BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:           return TextureFormat::BC3_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB:      return TextureFormat::BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC4_UNORM:           return TextureFormat::BC4_UNORM;
        case DXGI_FORMAT_BC5_UNORM:           return TextureFormat::BC5_UNORM;
        case DXGI_FORMAT_BC6H_UF16:           return TextureFormat::BC6H_UF16;
        case DXGI_FORMAT_BC6H_SF16:           return TextureFormat::BC6H_SF16;
        case DXGI_FORMAT_BC7_UNORM:           return TextureFormat::BC7_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB:      return TextureFormat::BC7_UNORM_SRGB;
        default:                              return TextureFormat::UNKNOWN;
        }
    }
}

namespace replicant::dds {

    DDSFile::DDSFile() : image_(std::make_unique<DirectX::ScratchImage>()) {}
    DDSFile::~DDSFile() = default;
    DDSFile::DDSFile(DDSFile&&) noexcept = default;
    DDSFile& DDSFile::operator=(DDSFile&&) noexcept = default;

    std::expected<DDSFile, DDSError> DDSFile::Load(const std::filesystem::path& path) {
        DDSFile file;
        DirectX::TexMetadata metadata;
        HRESULT hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, *file.image_);
        if (FAILED(hr)) {
            return std::unexpected(DDSError{ "DirectXTex LoadFromDDSFile failed" });
        }
        return file;
    }

    std::expected<DDSFile, DDSError> DDSFile::LoadFromMemory(std::span<const std::byte> data) {
        DDSFile file;
        DirectX::TexMetadata metadata;
        HRESULT hr = DirectX::LoadFromDDSMemory(data.data(), data.size(), DirectX::DDS_FLAGS_NONE, &metadata, *file.image_);
        if (FAILED(hr)) {
            return std::unexpected(DDSError{ "DirectXTex LoadFromDDSMemory failed" });
        }
        return file;
    }

    std::expected<DDSFile, DDSError> DDSFile::CreateFromGameData(
        const TextureHeader& header,
        std::span<const std::byte> pixelData
    ) {
        if (header.mipCount == 0) return std::unexpected(DDSError{ "Invalid mip count (0)" });

        DXGI_FORMAT dxgiFmt = XonToDXGI(header.format);
        if (dxgiFmt == DXGI_FORMAT_UNKNOWN) return std::unexpected(DDSError{ "Unsupported Texture Format" });

        DDSFile file;
        HRESULT hr;

        size_t arraySize = header.calculateArraySize();

        if (header.dimension == TextureDimension::Texture3D) {
            hr = file.image_->Initialize3D(dxgiFmt, header.width, header.height, header.depth, header.mipCount);
        }
        else {
            hr = file.image_->Initialize2D(dxgiFmt, header.width, header.height, arraySize, header.mipCount);
        }

        if (FAILED(hr)) return std::unexpected(DDSError{ "Failed to initialize ScratchImage" });

        for (size_t item = 0; item < arraySize; ++item) {
            for (size_t mip = 0; mip < header.mipCount; ++mip) {

                size_t flatIndex = item * header.mipCount + mip;
                if (flatIndex >= header.mips.size()) break;

                const auto& mipInfo = header.mips[flatIndex];

                const DirectX::Image* img = file.image_->GetImage(mip, item, 0);
                if (!img) continue;

                size_t dataSizeToCopy = mipInfo.sliceSize * mipInfo.depth;
                if (mipInfo.offset + dataSizeToCopy > pixelData.size()) {
                    return std::unexpected(DDSError{ "Mip data out of bounds" });
                }

                const uint8_t* src = reinterpret_cast<const uint8_t*>(pixelData.data()) + mipInfo.offset;
                uint8_t* dst = img->pixels;

                if (img->slicePitch == mipInfo.sliceSize && img->slicePitch * mipInfo.depth == dataSizeToCopy) {
                    memcpy(dst, src, dataSizeToCopy);
                }
                else {
                    // for cases where pitch alignment differs like 1px textures
                    size_t copySize = std::min((size_t)img->slicePitch * mipInfo.depth, (size_t)dataSizeToCopy);
                    memcpy(dst, src, copySize);
                }
            }
        }
        return file;
    }

    std::pair<TextureHeader, std::vector<std::byte>> DDSFile::ToGameFormat() const {
        const auto& meta = image_->GetMetadata();
        TextureHeader header;

        header.width = static_cast<uint32_t>(meta.width);
        header.height = static_cast<uint32_t>(meta.height);
        header.depth = static_cast<uint32_t>(meta.depth);
        header.mipCount = static_cast<uint32_t>(meta.mipLevels);
        header.format = DXGIToXon(meta.format);

        if (meta.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) {
            header.dimension = TextureDimension::Texture3D;
        }
        else if (meta.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) {
            header.dimension = TextureDimension::CubeMap;
        }
        else {
            header.dimension = TextureDimension::Texture2D;
        }

        std::vector<std::byte> pixelBlob;
        uint32_t currentOffset = 0;

        for (size_t item = 0; item < meta.arraySize; ++item) {
            for (size_t mip = 0; mip < meta.mipLevels; ++mip) {

                const DirectX::Image* img = image_->GetImage(mip, item, 0);
                if (!img) continue;

                MipSurface surface;
                surface.offset = currentOffset;

                size_t rowBytes, numRows, numBytes;
                DirectX::ComputePitch(meta.format, img->width, img->height, rowBytes, numRows, DirectX::CP_FLAGS_NONE);

                surface.rowPitch = static_cast<uint32_t>(rowBytes);
                surface.rowCount = static_cast<uint32_t>(numRows);
                surface.sliceSize = static_cast<uint32_t>(img->slicePitch);

                surface.width = static_cast<uint32_t>(img->width);
                surface.height = static_cast<uint32_t>(img->height);

                if (meta.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) {
                    size_t mipDepth = meta.depth >> mip;
                    if (mipDepth < 1) mipDepth = 1;
                    surface.depth = static_cast<uint32_t>(mipDepth);
                }
                else {
                    surface.depth = 1;
                }

                header.mips.push_back(surface);

                size_t totalBytesForSubresource = img->slicePitch * surface.depth;
                const std::byte* srcBytes = reinterpret_cast<const std::byte*>(img->pixels);

                pixelBlob.insert(pixelBlob.end(), srcBytes, srcBytes + totalBytesForSubresource);
                currentOffset += static_cast<uint32_t>(totalBytesForSubresource);
            }
        }

        header.totalPixelSize = static_cast<uint32_t>(pixelBlob.size());
        return { header, pixelBlob };
    }

    uint32_t DDSFile::getWidth() const { return static_cast<uint32_t>(image_->GetMetadata().width); }
    uint32_t DDSFile::getHeight() const { return static_cast<uint32_t>(image_->GetMetadata().height); }
    uint32_t DDSFile::getDepth() const { return static_cast<uint32_t>(image_->GetMetadata().depth); }
    uint32_t DDSFile::getMipLevels() const { return static_cast<uint32_t>(image_->GetMetadata().mipLevels); }
    TextureFormat DDSFile::getFormat() const { return DXGIToXon(image_->GetMetadata().format); }

    std::expected<void, DDSError> DDSFile::Save(const std::filesystem::path& path) {
        if (image_->GetImageCount() == 0) return std::unexpected(DDSError{ "Empty image" });
        HRESULT hr = DirectX::SaveToDDSFile(image_->GetImages(), image_->GetImageCount(), image_->GetMetadata(), DirectX::DDS_FLAGS_NONE, path.c_str());
        if (FAILED(hr)) return std::unexpected(DDSError{ "SaveToDDSFile failed" });
        return {};
    }

    std::expected<std::vector<std::byte>, DDSError> DDSFile::SaveToMemory() {
        if (image_->GetImageCount() == 0) return std::unexpected(DDSError{ "Empty image" });
        DirectX::Blob blob;
        HRESULT hr = DirectX::SaveToDDSMemory(image_->GetImages(), image_->GetImageCount(), image_->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);
        if (FAILED(hr)) return std::unexpected(DDSError{ "SaveToDDSMemory failed" });

        const std::byte* ptr = reinterpret_cast<const std::byte*>(blob.GetBufferPointer());
        return std::vector<std::byte>(ptr, ptr + blob.GetBufferSize());
    }

    std::span<const std::byte> DDSFile::getPixelData() const {
        if (!image_ || image_->GetPixelsSize() == 0) return {};
        return std::span<const std::byte>(reinterpret_cast<const std::byte*>(image_->GetPixels()), image_->GetPixelsSize());
    }
}