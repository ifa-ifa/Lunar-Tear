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


    class ArcWriter {
    public:
        struct Entry {
            std::string key;
            size_t uncompressed_size;
            size_t compressed_size; // Note: This will be the size of the entire compressed archive
            uint64_t offset; // Offset in the uncompressed data blob
        };

        std::expected<void, ArcError> addFile(std::string key, std::vector<char> data);
        std::expected<void, ArcError> addFileFromDisk(std::string key, const std::string& filepath);

        std::expected<std::vector<char>, ArcError> build(int compression_level = 1);

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