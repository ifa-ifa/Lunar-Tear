#include "replicant/arc.h"
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include <iostream>
#include <algorithm>
#include <cstring>

namespace replicant::archive {

    namespace {
        constexpr size_t SECTOR_ALIGNMENT = 16;

        size_t align_to(size_t value, size_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }
    }

    std::expected<std::vector<std::byte>, Error> Compress(std::span<const std::byte> data, CompressionConfig config) {
        if (data.empty()) return std::vector<std::byte>{};
        size_t bound = ZSTD_compressBound(data.size());
        std::vector<std::byte> buffer(bound);

        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) return std::unexpected(Error{ ErrorCode::SystemError, "Failed to create ZSTD Context" });

        ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, config.level);
        ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, config.windowLog);

        size_t cSize = ZSTD_compress2(
            cctx,
            buffer.data(),
            bound,
            data.data(),
            data.size()
        );

        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(cSize)) return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(cSize) });
        buffer.resize(cSize);
        return buffer;
    }


    std::expected<std::vector<std::byte>, Error> Decompress(std::span<const std::byte> data, size_t decompressedSize) {
        if (data.empty()) return std::vector<std::byte>{};

        std::vector<std::byte> output(decompressedSize);
        size_t dSize = ZSTD_decompress(output.data(), decompressedSize, data.data(), data.size());

        if (ZSTD_isError(dSize)) {
            return std::unexpected(Error{ ErrorCode::SystemError, ZSTD_getErrorName(dSize) });
        }
        if (dSize != decompressedSize) {
            return std::unexpected(Error{ ErrorCode::InvalidArguments, "Decompressed size mismatch" });
        }
        return output;
    }

    std::expected<size_t, Error> GetDecompressedSize(std::span<const std::byte> data) {
        unsigned long long size = ZSTD_getFrameContentSize(data.data(), data.size());
        if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN) {
            return std::unexpected(Error{ ErrorCode::SystemError, "Could not determine frame content size" });
        }
        return static_cast<size_t>(size);
    }

    std::expected<ArchiveResult, Error> Build(
        const std::vector<ArchiveInput>& inputs,
        BuildMode mode,
        CompressionConfig config
    ) {
        ArchiveResult result;
        result.entries.reserve(inputs.size());

        if (mode == BuildMode::SingleStream) {
            // PRELOAD_DECOMPRESS (Type 0)
            // Concatenate uncompressed -> Record Offsets -> Compress Whole

            size_t totalUncompressed = 0;
            for (const auto& input : inputs) totalUncompressed += input.data.size();

            std::vector<std::byte> hugeBuffer;
            hugeBuffer.reserve(totalUncompressed);

            size_t currentOffset = 0;
            for (const auto& input : inputs) {

                size_t padding_needed = (16 - (currentOffset % 16)) % 16;
                if (padding_needed > 0) {
                    hugeBuffer.insert(hugeBuffer.end(), padding_needed, std::byte{ 0 });
                    currentOffset += padding_needed;
                }

                hugeBuffer.insert(hugeBuffer.end(), input.data.begin(), input.data.end());

                ArchiveEntryInfo entry;
                entry.name = input.name;

                // For Type 0, offset is the position in the DECOMPRESSED memory
                entry.offset = currentOffset;

                // For Type 0, compressedSize is the size to READ from memory (Uncompressed Size)
                entry.compressedSize = 0;

                entry.packSerializedSize = input.packSerializedSize;
                entry.packResourceSize = input.packResourceSize;

                result.entries.push_back(entry);

                currentOffset += input.data.size();
            }

            auto compressRes = Compress(hugeBuffer, config);
            if (!compressRes) return std::unexpected(compressRes.error());

            result.arcData = std::move(*compressRes);
        }
        else {
            // STREAM (Type 1/2)
            // Compress each -> Align -> Record Offsets

            for (const auto& input : inputs) {
                auto compressRes = Compress(input.data, config);
                if (!compressRes) return std::unexpected(compressRes.error());

                size_t cSize = compressRes->size();
                size_t alignedSize = align_to(cSize, SECTOR_ALIGNMENT);
                size_t padding = alignedSize - cSize;

                ArchiveEntryInfo entry;
                entry.name = input.name;

                // For Type 1/2, offset is the position in the ARC file
                entry.offset = result.arcData.size();

                // For Type 1/2, compressedSize is the ZSTD frame size
                entry.compressedSize = static_cast<uint32_t>(cSize);

                entry.packSerializedSize = input.packSerializedSize;
                entry.packResourceSize = input.packResourceSize;

                result.entries.push_back(entry);

                result.arcData.insert(result.arcData.end(), compressRes->begin(), compressRes->end());
                if (padding > 0) {
                    result.arcData.insert(result.arcData.end(), padding, std::byte{ 0 });
                }
            }
        }

        return result;
    }
}