#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>
#include <expected> 
#include <span>




namespace replicant::archive {

    enum class ArcErrorCode {
        Success,
        ZstdCompressionError,
        ZstdDecompressionError,
        DecompressionSizeMismatch,
        EmptyInput,
        DuplicateKey,
        FileReadError,
        Unimplemented
    };

    struct ArcError {
        ArcErrorCode code;
        std::string message;
    };

    /*!
     * @brief Decompresses a ZSTD frame.
     * @param compressed_data Pointer to the start of the compressed data buffer.
     * @param compressed_size The size of the compressed data.
     * @param decompressed_size The known, exact size of the original uncompressed data.
     * @return The decompressed data vector
     */
    std::expected<std::vector<char>, ArcError> decompress_zstd(
        const void* compressed_data,
        size_t compressed_size,
        size_t decompressed_size
    );

    /*!
     * @brief Compresses data ito a single ZSTD frame.
     * @param uncompressed_data Pointer to the start of the data to compress.
     * @param uncompressed_size The size of the data to compress.
     * @param compression_level The ZSTD compression level 
     * @return The compressed data vector 
     */
    std::expected<std::vector<char>, ArcError> compress_zstd(
        const void* uncompressed_data,
        size_t uncompressed_size,
        int compression_level = 1
    );


    /*!
     * @brief Gets the original, uncompressed size of a ZSTD frame.
     * @param compressed_data A span containing the compressed ZSTD frame data.
     * @return The decompressed size
     */
    std::expected<size_t, ArcError> get_decompressed_size_zstd(
        std::span<const char> compressed_data
    );

    /// @brief Defines the structure of the output .arc file.
    enum class ArcBuildMode {
   
        SingleStream,   /// @brief All files are concatenated and compressed together in one stream. Used for PRELOAD_DECOMPRESS.
        ConcatenatedFrames   ///@brief Each file is compressed into a separate, aligned ZSTD frame. Used for STREAM types.
    };

    /*!
     * @brief A builder class for creating .arc archive files.
     *
     * To create a full arc:
     * 1. Create an ArcWriter instance
     * 2. Add files using addFile() or addFileFromDisk()
     * 3. build()
     * 4. Call getEntries() to get the metadata required to generate a index file
     */
    class ArcWriter {
    public:
        struct Entry {
            std::string key;
            size_t PackSerialisedSize;
            size_t compressed_size;
            uint64_t offset;
            uint32_t assetsDataSize;
        };

        /*!
         * @brief Adds a file from in memory to the archive.
         */
        std::expected<void, ArcError> addFile(
            std::string key,
            std::vector<char> data,
            uint32_t serialized_size,
            uint32_t resource_size
        );

        std::expected<void, ArcError> addFileFromDisk(std::string key, const std::string& filepath);

        std::expected<std::vector<char>, ArcError> build(
            ArcBuildMode mode,
            int compression_level = 1
        );

        const std::vector<Entry>& getEntries() const;

        /*!
         * @brief Gets the number of files currently waiting to be built.
         * @return The number of pending files.
         */
        size_t getPendingEntryCount() const;

    private:
        struct PendingEntry {
            std::string key;
            std::vector<char> uncompressed_data;
            uint32_t serialized_size; 
            uint32_t resource_size;  

        };

        std::vector<PendingEntry> m_pending_entries;
        std::vector<Entry> m_built_entries;
    };

}