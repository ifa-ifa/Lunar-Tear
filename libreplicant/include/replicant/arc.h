#pragma once
#include "replicant/core/common.h"
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>
#include <span>
#include <expected>

namespace replicant::archive {

    struct CompressionConfig {
        int level = 1;
        int windowLog = 15; // Game will crash if any higher
    };

    enum class BuildMode {
        SingleStream,   // PRELOAD_DECOMPRESS (Type 0)
        SeparateFrames  // STREAM / STREAM_ONDEMAND (Type 1/2)
    };

    struct ArchiveInput {
        std::string name;
        std::vector<std::byte> data;
        uint32_t packSerializedSize = 0;
        uint32_t packResourceSize = 0;
    };

    struct ArchiveEntryInfo {
        std::string name;

        // For SingleStream: Offset into the decompressed buffer
        // For SeparateFrames: Offset into the .arc file
        uint64_t offset;

        uint32_t compressedSize;
        uint32_t packSerializedSize;
        uint32_t packResourceSize;
    };

    struct ArchiveResult {
        std::vector<std::byte> arcData;
        std::vector<ArchiveEntryInfo> entries;
    };

    std::expected<std::vector<std::byte>, Error> Compress(
        std::span<const std::byte> data,
        CompressionConfig config = {}
    );

    std::expected<std::vector<std::byte>, Error> Decompress(
        std::span<const std::byte> compressedData,
        size_t decompressedSize
    );

    std::expected<size_t, Error> GetDecompressedSize(
        std::span<const std::byte> compressedData
    );


    std::expected<ArchiveResult, Error> Build(
        const std::vector<ArchiveInput>& inputs,
        BuildMode mode,
        CompressionConfig config = {}
    );
    
}