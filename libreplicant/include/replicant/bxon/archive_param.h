#pragma once

#include <string>
#include <vector>
#include <cstdint>

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
        uint32_t decompressedSize = 0;
        uint32_t bufferSize = 0;
        uint8_t archiveIndex = 0;
        uint8_t flags = 0;
    };

    class ArchiveFileParam {
    public:
        std::vector<ArchiveEntry>& getArchiveEntries() { return m_archive_entries; }
        const std::vector<ArchiveEntry>& getArchiveEntries() const { return m_archive_entries; }
        std::vector<FileEntry>& getFileEntries() { return m_file_entries; }
        const std::vector<FileEntry>& getFileEntries() const { return m_file_entries; }

        FileEntry* findFileEntryMutable(const std::string& key);

        uint8_t addArchive(std::string filename, ArchiveLoadType loadType = ArchiveLoadType::PRELOAD_DECOMPRESS);

        friend class File;
    private:
        std::vector<ArchiveEntry> m_archive_entries;
        std::vector<FileEntry> m_file_entries;
    };

} 