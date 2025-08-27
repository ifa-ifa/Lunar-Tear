#pragma once
#include "replicant/arc.h"
#include "replicant/bxon/error.h"
#include <string>
#include <vector>
#include <cstdint>
#include <expected>

namespace replicant::bxon {

    enum class ArchiveLoadType : uint8_t {
        PRELOAD_DECOMPRESS = 0,
        STREAM = 1,
        STREAM_SPECIAL = 2
    };

    struct ArchiveEntry {
        std::string filename;
        ArchiveLoadType loadType;
        uint32_t offsetScale = 4;
    };

    struct FileEntry {
        uint32_t pathHash = 0;
        std::string filePath;
        uint32_t arcOffset = 0; // Uncompressed offset, bit-shifted by ArchiveEntry::offsetScale
        uint32_t compressedSize = 0;
        uint32_t PackSerialisedSize = 0;
        uint32_t assetsDataSize = 0;
        uint8_t archiveIndex = 0;
        uint8_t flags = 0;
    };

    /// @brief Represents a 'tpArchiveFileParam' BXON asset, which acts as an index for .arc files.
    class ArchiveParameters {
    public:

        static ArchiveParameters FromArcEntries(
            const std::vector<archive::ArcWriter::Entry> & entries,
            uint8_t archive_index,
            const std::string& archive_filename,
            ArchiveLoadType load_type = ArchiveLoadType::PRELOAD_DECOMPRESS
        );

        std::expected<std::vector<char>, BxonError> build(uint32_t version, uint32_t projectID) const;

        std::vector<ArchiveEntry>& getArchiveEntries() { return m_archive_entries; }
        const std::vector<ArchiveEntry>& getArchiveEntries() const { return m_archive_entries; }
        std::vector<FileEntry>& getFileEntries() { return m_file_entries; }
        const std::vector<FileEntry>& getFileEntries() const { return m_file_entries; }

        FileEntry* findFile(const std::string& key);
        uint8_t addArchive(std::string filename, ArchiveLoadType loadType = ArchiveLoadType::PRELOAD_DECOMPRESS);

        friend class File;
    private:
        std::vector<ArchiveEntry> m_archive_entries;
        std::vector<FileEntry> m_file_entries;
    };

}