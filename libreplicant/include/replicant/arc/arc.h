#pragma once

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>
#include <expected> 

#include "replicant/arc/error.h"

namespace replicant::archive {


    std::expected<std::vector<char>, ArcError> decompress_zstd(
        const void* compressed_data,
        size_t compressed_size,
        size_t decompressed_size
    );

    std::expected<std::vector<char>, ArcError> compress_zstd(
        const void* uncompressed_data,
        size_t uncompressed_size,
        int compression_level = 1 
    );


    enum class ArcBuildMode {
        // For LOAD_PRELOAD_DECOMPRESS: Concatenate all files raw, then compress the entire blob once.
        // Offsets will be relative to the uncompressed data.
        SingleStream,

        // For LOAD_STREAM and LOAD_STREAM_ONDEMAND: Individually compress each file and concatenate the results.
        // Offsets will be the physical offset of each compressed chunk.
        ConcatenatedFrames
    };

    class ArcWriter {
    public:
        struct Entry {
            std::string key;
            size_t everythingExceptAssetsDataSize;
            size_t compressed_size; 
            uint64_t offset;
            uint32_t assetsDataSize;
        };

        std::expected<void, ArcError> addFile(std::string key, std::vector<char> data);
        std::expected<void, ArcError> addFileFromDisk(std::string key, const std::string& filepath);

        std::expected<std::vector<char>, ArcError> build(
            ArcBuildMode mode,
            int compression_level = 1
        );

        const std::vector<Entry>& getEntries() const;

    private:
        struct PendingEntry {
            std::string key;
            std::vector<char> uncompressed_data;
        };

        std::vector<PendingEntry> m_pending_entries;
        std::vector<Entry> m_built_entries;
    };

}