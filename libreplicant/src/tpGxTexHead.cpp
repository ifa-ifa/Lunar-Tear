#include "replicant/tpGxTexHead.h"
#include "replicant/core/writer.h"
using namespace replicant::raw;

namespace replicant {

    std::expected<TextureHeader, ReaderError> Texture::DeserializeHeader(std::span<const std::byte> data) {
        Reader reader(data);

        auto rawRes = reader.view<RawTexHeader>();
        if (!rawRes) return std::unexpected(rawRes.error());
        const auto* raw = *rawRes;

        TextureHeader header;
        header.width = raw->width;
        header.height = raw->height;
        header.depth = raw->depth;
        header.mipCount = raw->mipCount;
        header.totalPixelSize = raw->size;
        header.format = raw->format.resourceFormat;
        header.dimension = raw->format.resourceDimension;

        auto mipArrayRes = reader.getOffsetPtr(raw->offsetToSubresources);
        if (!mipArrayRes) return std::unexpected(mipArrayRes.error());

        const std::byte* mipStart = reinterpret_cast<const std::byte*>(*mipArrayRes);
        if (mipStart + (raw->subresourcesCount * sizeof(RawMipSurface)) > data.data() + data.size()) {
            return std::unexpected(ReaderError{ ReaderErrorCode::OutOfBounds, "Mip table exceeds buffer" });
        }

        const RawMipSurface* rawMips = reinterpret_cast<const RawMipSurface*>(*mipArrayRes);

        header.mips.reserve(raw->subresourcesCount);
        for (uint32_t i = 0; i < raw->subresourcesCount; i++) {
            MipSurface mip;
            mip.offset = rawMips[i].offset;
            mip.rowPitch = rawMips[i].rowPitch;
            mip.sliceSize = rawMips[i].sliceSize;
            mip.width = rawMips[i].width;
            mip.height = rawMips[i].height;
            mip.depth = rawMips[i].depth;
            mip.rowCount = rawMips[i].rowCount;
            header.mips.push_back(mip);
        }

        return header;
    }

    std::vector<std::byte> Texture::SerializeHeader(const TextureHeader& header) {
        Writer writer;

        writer.write(header.width);
        writer.write(header.height);
        writer.write(header.depth);
        writer.write(header.mipCount);
        writer.write(header.totalPixelSize);
        writer.write<uint32_t>(0x80000000); // unknownOffset

        RawSurfaceFormat fmt = {};
        fmt.resourceFormat = header.format;
        fmt.resourceDimension = header.dimension;
        fmt.usageMaybe = 0;
        fmt.generateMips = 0; 
        writer.write(fmt);

        writer.write<uint32_t>(static_cast<uint32_t>(header.mips.size()));
        size_t tokenMips = writer.reserveOffset();

        writer.align(16);
        writer.satisfyOffsetHere(tokenMips);

        for (const auto& mip : header.mips) {
            writer.write(mip.offset);
            writer.write<uint32_t>(0);
            writer.write(mip.rowPitch);
            writer.write<uint32_t>(0);
            writer.write(mip.sliceSize);
            writer.write<uint32_t>(0);
            writer.write(mip.width);
            writer.write(mip.height);
            writer.write(mip.depth);
            writer.write(mip.rowCount);
        }

        return writer.buffer();
    }
}