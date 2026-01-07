#include "replicant/tpGxTexHead.h"
#include "replicant/core/writer.h"
#include "replicant/core/reader.h"

using namespace replicant::raw;

namespace replicant {

    TextureHeader DeserializeTexHeadInternal(std::span<const std::byte> data) {

        Reader reader(data);

        auto raw = reader.view<RawTexHeader>();

        TextureHeader header;
        header.width = raw->width;
        header.height = raw->height;
        header.depth = raw->depth;
        header.mipCount = raw->mipCount;
        header.totalPixelSize = raw->size;
        header.format = raw->format.resourceFormat;
        header.dimension = raw->format.resourceDimension;

        auto mipArray = reader.getOffsetPtr(raw->offsetToSubresources);

        const std::byte* mipStart = reinterpret_cast<const std::byte*>(mipArray);
        if (mipStart + (raw->subresourcesCount * sizeof(RawMipSurface)) > data.data() + data.size()) {
			throw ReaderException("Texture header mip array exceeds buffer bounds");
        }

        const RawMipSurface* rawMips = reinterpret_cast<const RawMipSurface*>(mipArray);

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

    std::expected<TextureHeader, Error> DeserializeTexHead(std::span<const std::byte> data) {

        try {
            return DeserializeTexHeadInternal(data);
        }
        catch (const ReaderException& e) {
            return std::unexpected(Error{ ErrorCode::ParseError, e.what() });
        }
        catch (const std::exception& e) {
            return std::unexpected(Error{ ErrorCode::SystemError,
                std::string("Unexpected error while deserializing texture header: ") + e.what() });
		}

    }

    std::vector<std::byte> SerializeTexHeadInternal(const TextureHeader& header) {
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

    std::expected<std::vector<std::byte>, Error> SerializeTexHead(const TextureHeader& header) {
        try {
            return SerializeTexHeadInternal(header);
        }
        catch (const std::exception& e) {
            return std::unexpected(Error{ ErrorCode::SystemError,
                std::string("Unexpected error while serializing texture header: ") + e.what() });
        }
    }
}