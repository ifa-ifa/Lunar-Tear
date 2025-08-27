#include "replicant/arc.h"
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include <fstream>
#include "replicant/pack.h"
#include <algorithm>
#include <system_error>
#include <cerrno>

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

    std::expected<std::vector<char>, ArcError> compress_zstd(
        const void* uncompressed_data,
        size_t uncompressed_size,
        int compression_level
    ) {
        if (!uncompressed_data || uncompressed_size == 0) {
            return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "Input data cannot be empty." });
        }

        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) {
            return std::unexpected(ArcError{ ArcErrorCode::ZstdCompressionError, "Failed to create ZSTD context" });
        }

        ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compression_level);

        // The game will crash if the window size is any larger
        ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, 15); 

        size_t max_compressed_size = ZSTD_compressBound(uncompressed_size);
        std::vector<char> compressed_buffer(max_compressed_size);

        size_t result = ZSTD_compress2(
            cctx,
            compressed_buffer.data(),
            compressed_buffer.size(),
            uncompressed_data,
            uncompressed_size
        );

        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(result)) {
            return std::unexpected(ArcError{ ArcErrorCode::ZstdCompressionError, ZSTD_getErrorName(result) });
        }

        compressed_buffer.resize(result);
        return compressed_buffer;
    }

    size_t ArcWriter::getPendingEntryCount() const {
        return m_pending_entries.size();
    }


    std::expected<size_t, ArcError> get_decompressed_size_zstd(std::span<const char> compressed_data) {
        if (compressed_data.empty()) {
            return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "Input data cannot be empty." });
        }

        unsigned long long const size = ZSTD_getFrameContentSize(compressed_data.data(), compressed_data.size());

        if (size == ZSTD_CONTENTSIZE_ERROR) {
            return std::unexpected(ArcError{ ArcErrorCode::ZstdDecompressionError, "Data is not in a valid ZSTD format." });
        }
        if (size == ZSTD_CONTENTSIZE_UNKNOWN) {
            return std::unexpected(ArcError{ ArcErrorCode::ZstdDecompressionError, "Decompressed size could not be determined from the frame header." });
        }

        return static_cast<size_t>(size);
    }

    std::expected<void, ArcError> ArcWriter::addFile(
        std::string key,
        std::vector<char> data,
        uint32_t serialized_size,
        uint32_t resource_size
    ) {
        if (key.empty() || data.empty()) {
            return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "File key or data cannot be empty." });
        }
        for (const auto& entry : m_pending_entries) {
            if (entry.key == key) {
                return std::unexpected(ArcError{ ArcErrorCode::DuplicateKey, "Duplicate key added to writer: " + key });
            }
        }
        m_pending_entries.emplace_back(PendingEntry{
            std::move(key),
            std::move(data),
            serialized_size,
            resource_size
            });
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

        pack::PackFile packfile;
        auto status = packfile.loadFromMemory(data.data(), data.size());
     if (!status) {
        return std::unexpected(ArcError{ 
            ArcErrorCode::FileReadError,
            "Failed to parse as PACK file '" + filepath + "'. Reason: " + status.error().message 
        });
    }

        return addFile(
            std::move(key),
            std::move(data),
            packfile.getSerializedSize(),
            packfile.getResourceSize()
        );
    }

    std::expected<std::vector<char>, ArcError> ArcWriter::build(ArcBuildMode mode, int compression_level) {
        m_built_entries.clear();

        if (mode == ArcBuildMode::SingleStream) {
            std::vector<char> uncompressed_blob;
            uint64_t current_uncompressed_offset = 0;

            for (const auto& pending_entry : m_pending_entries) {
                Entry built_entry;
                built_entry.key = pending_entry.key;
                built_entry.assetsDataSize = pending_entry.resource_size;
                built_entry.PackSerialisedSize = pending_entry.serialized_size;
                built_entry.offset = current_uncompressed_offset;
                built_entry.compressed_size = 0;
                m_built_entries.push_back(built_entry);

                uncompressed_blob.insert(uncompressed_blob.end(), pending_entry.uncompressed_data.begin(), pending_entry.uncompressed_data.end());
                current_uncompressed_offset += pending_entry.uncompressed_data.size();
            }
            return compress_zstd(uncompressed_blob.data(), uncompressed_blob.size(), compression_level);
        }
        else { // mode == ArcBuildMode::ConcatenatedFrames
            std::vector<char> arc_data;
            uint64_t current_compressed_offset = 0;
            constexpr size_t ALIGNMENT = 16;

            for (const auto& pending_entry : m_pending_entries) {
                auto compressed_result = compress_zstd(pending_entry.uncompressed_data.data(), pending_entry.uncompressed_data.size(), compression_level);
                if (!compressed_result) {
                    return std::unexpected(compressed_result.error());
                }
                std::vector<char>& compressed_data = *compressed_result;

                size_t start_padding = (ALIGNMENT - (current_compressed_offset % ALIGNMENT)) % ALIGNMENT;
                if (start_padding > 0) {
                    arc_data.insert(arc_data.end(), start_padding, 0x00);
                    current_compressed_offset += start_padding;
                }

                uint64_t entry_offset = current_compressed_offset;
                arc_data.insert(arc_data.end(), compressed_data.begin(), compressed_data.end());

                size_t unpadded_size = compressed_data.size();
                size_t padded_size = (unpadded_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
                size_t end_padding = padded_size - unpadded_size;

                if (end_padding > 0) {
                    arc_data.insert(arc_data.end(), end_padding, 0x00);
                }
                current_compressed_offset += padded_size;

                Entry built_entry;
                built_entry.key = pending_entry.key;
                built_entry.compressed_size = unpadded_size;
                built_entry.offset = entry_offset;
                built_entry.PackSerialisedSize = pending_entry.serialized_size;
                built_entry.assetsDataSize = pending_entry.resource_size;
                m_built_entries.push_back(built_entry);
            }
            return arc_data;
        }
    }

    const std::vector<ArcWriter::Entry>& ArcWriter::getEntries() const {
        return m_built_entries;
    }
}