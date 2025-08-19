#include "replicant/arc/arc.h"
#include <zstd.h>
#include <fstream>
#include <algorithm>
#include <system_error>
#include <cerrno>
#include <numeric>

namespace replicant::archive {

    std::expected<std::vector<char>, ArcError> decompress_zstd(const void* compressed_data, size_t compressed_size, size_t decompressed_size) {
        if (!compressed_data || compressed_size == 0 || decompressed_size == 0) {
            return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "Input data or sizes cannot be zero." });
        }
        std::vector<char> decompressed_buffer(decompressed_size);
        const size_t result = ZSTD_decompress(decompressed_buffer.data(), decompressed_buffer.size(), compressed_data, compressed_size);
        if (ZSTD_isError(result)) {
            return std::unexpected(ArcError{ ArcErrorCode::ZstdDecompressionError, ZSTD_getErrorName(result) });
        }
        if (result != decompressed_size) {
            return std::unexpected(ArcError{ ArcErrorCode::DecompressionSizeMismatch, "Decompressed data size did not match expected size." });
        }
        return decompressed_buffer;
    }

    std::expected<std::vector<char>, ArcError> compress_zstd(const void* uncompressed_data, size_t uncompressed_size, int compression_level) {
        if (!uncompressed_data || uncompressed_size == 0) {
            return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "Input data cannot be empty." });
        }
        const size_t max_compressed_size = ZSTD_compressBound(uncompressed_size);
        std::vector<char> compressed_buffer(max_compressed_size);
        const size_t result = ZSTD_compress(compressed_buffer.data(), compressed_buffer.size(), uncompressed_data, uncompressed_size, compression_level);
        if (ZSTD_isError(result)) {
            return std::unexpected(ArcError{ ArcErrorCode::ZstdCompressionError, ZSTD_getErrorName(result) });
        }
        compressed_buffer.resize(result);
        return compressed_buffer;
    }

    std::expected<void, ArcError> ArcWriter::addFile(std::string key, std::vector<char> data) {
        if (key.empty()) { return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "File key cannot be empty." }); }
        if (data.empty()) { return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "File data cannot be empty." }); }
        for (const auto& entry : m_pending_entries) {
            if (entry.key == key) {
                return std::unexpected(ArcError{ ArcErrorCode::DuplicateKey, "Duplicate key added to writer: " + key });
            }
        }
        m_pending_entries.emplace_back(PendingEntry{ std::move(key), std::move(data) });
        return {};
    }

    std::expected<void, ArcError> ArcWriter::addFileFromDisk(std::string key, const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            return std::unexpected(ArcError{ ArcErrorCode::FileReadError, "Could not open file '" + filepath + "': " + std::strerror(errno) });
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> data(size);
        if (!file.read(data.data(), size)) {
            return std::unexpected(ArcError{ ArcErrorCode::FileReadError, "Failed to read all data from file: " + filepath });
        }
        return addFile(std::move(key), std::move(data));
    }

    std::expected<std::vector<char>, ArcError> ArcWriter::build(int compression_level) {
        m_built_entries.clear();
        if (m_pending_entries.empty()) {
            return std::vector<char>(); 
        }

        // 1) Concatenate all uncompressed files into one large blob.
        std::vector<char> uncompressed_blob;
        size_t total_uncompressed_size = 0;
        for (const auto& entry : m_pending_entries) {
            total_uncompressed_size += entry.uncompressed_data.size();
        }
        uncompressed_blob.reserve(total_uncompressed_size);

        uint64_t current_uncompressed_offset = 0;
        for (const auto& pending_entry : m_pending_entries) {
            Entry built_entry;
            built_entry.key = pending_entry.key;
            built_entry.uncompressed_size = pending_entry.uncompressed_data.size();
            built_entry.offset = current_uncompressed_offset;
            // compressed_size will be filled in later for all entries at once
            m_built_entries.push_back(built_entry);

            uncompressed_blob.insert(uncompressed_blob.end(), pending_entry.uncompressed_data.begin(), pending_entry.uncompressed_data.end());
            current_uncompressed_offset += pending_entry.uncompressed_data.size();
        }

        auto compressed_result = compress_zstd(uncompressed_blob.data(), uncompressed_blob.size(), compression_level);
        if (!compressed_result) {
            return std::unexpected(compressed_result.error());
        }

        const size_t final_compressed_size = compressed_result->size();
        for (auto& entry : m_built_entries) {
            entry.compressed_size = final_compressed_size;
        }
        return compressed_result;
    }

    const std::vector<ArcWriter::Entry>& ArcWriter::getEntries() const {
        return m_built_entries;
    }
}