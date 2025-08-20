#include "replicant/arc/arc.h"
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

    std::expected<std::vector<char>, ArcError> compress_zstd_with_long15(
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

        // Set window log to 15 (equivalent to --long=15, which gives 2^15 = 32KB window) TODO: MAKE THIS A CMD ARGUMENT
        // Never set this higher than 15, game will not load it
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
        if (key.empty() || data.empty()) {
            return std::unexpected(ArcError{ ArcErrorCode::EmptyInput, "File key or data cannot be empty." });
        }
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

    std::expected<std::vector<char>, ArcError> ArcWriter::build(ArcBuildMode mode, int compression_level) {
        m_built_entries.clear();

        if (mode == ArcBuildMode::SingleStream) {
            
            std::vector<char> uncompressed_blob;
            uint64_t current_uncompressed_offset = 0;

            for (const auto& pending_entry : m_pending_entries) {

                pack::PackFile packfile;
                bool status = packfile.loadFromMemory(pending_entry.uncompressed_data.data(), pending_entry.uncompressed_data.size());

                if (!status) {
                    return std::unexpected(ArcError{ ArcErrorCode::FileReadError, "Failed to read pack file" });
                }


                Entry built_entry;
                built_entry.key = pending_entry.key;


                built_entry.assetsDataSize = packfile.getResourceSize();       

                built_entry.everythingExceptAssetsDataSize = packfile.getTotalSize() - packfile.getResourceSize();

                built_entry.offset = current_uncompressed_offset;
                // In this mode, individual compressed size is not meaningful, but we can store it for metadata.
                // A real implementation might compress temporarily to get the size, but for now we set to 0.
                built_entry.compressed_size = 0;
                m_built_entries.push_back(built_entry);

                uncompressed_blob.insert(uncompressed_blob.end(), pending_entry.uncompressed_data.begin(), pending_entry.uncompressed_data.end());
                current_uncompressed_offset += pending_entry.uncompressed_data.size();
            }

            return compress_zstd(uncompressed_blob.data(), uncompressed_blob.size(), compression_level);

        }
        else { // mode == ArcBuildMode::ConcatenatedFrames
            // Mode for LOAD_STREAM
            std::vector<char> arc_data;
            uint64_t current_compressed_offset = 0;
            constexpr size_t ALIGNMENT = 16;

            for (const auto& pending_entry : m_pending_entries) {
                auto compressed_result = compress_zstd_with_long15(pending_entry.uncompressed_data.data(), pending_entry.uncompressed_data.size(), compression_level);
                if (!compressed_result) {
                    return std::unexpected(compressed_result.error());
                }
                std::vector<char>& compressed_data = *compressed_result;

                size_t padding_needed = (ALIGNMENT - (current_compressed_offset % ALIGNMENT)) % ALIGNMENT;
                if (padding_needed > 0) {
                    arc_data.insert(arc_data.end(), padding_needed, 0x00);
                    current_compressed_offset += padding_needed;
                }

                Entry built_entry;
                built_entry.key = pending_entry.key;
                built_entry.compressed_size = compressed_data.size();
                built_entry.offset = current_compressed_offset; // Physical offset

                pack::PackFile packfile;
                bool status = packfile.loadFromMemory(pending_entry.uncompressed_data.data(), pending_entry.uncompressed_data.size());

                if (!status) {
                    return std::unexpected(ArcError{ ArcErrorCode::FileReadError, "Failed to read pack file" });
                }
            
                built_entry.everythingExceptAssetsDataSize = packfile.getTotalSize() - packfile.getResourceSize(); 
                built_entry.assetsDataSize = packfile.getResourceSize();

                m_built_entries.push_back(built_entry);

                arc_data.insert(arc_data.end(), compressed_data.begin(), compressed_data.end());
                current_compressed_offset += compressed_data.size();
            }
            return arc_data;
        }
    }

    const std::vector<ArcWriter::Entry>& ArcWriter::getEntries() const {
        return m_built_entries;
    }
}


