#pragma once
#include "replicant/core/common.h"
#include "replicant/arc.h" 
#include <vector>
#include <string>
#include <cstdint>
#include <expected>

namespace replicant {

    enum class ArchiveLoadType : uint8_t {
        PRELOAD_DECOMPRESS = 0,
        STREAM = 1,
        STREAM_ONDEMAND = 2
    };

    struct ArchiveEntry {
        std::string filename;
        uint32_t arcOffsetScale = 4; 
        ArchiveLoadType loadType = ArchiveLoadType::PRELOAD_DECOMPRESS;
    };

    struct FileEntry {
        std::string name;


        uint64_t rawOffset = 0; // with scale already applied

        uint32_t size = 0;

        uint32_t packFileSerializedSize = 0;
        uint32_t packFileResourceSize = 0;

        uint8_t archiveIndex = 0;
        uint8_t flags = 0;
    };

    class TpArchiveFileParam {
    public:
        std::vector<ArchiveEntry> archiveEntries;
        std::vector<FileEntry> fileEntries;

        static std::expected<TpArchiveFileParam, Error> Deserialize(std::span<const std::byte> payload);
        std::expected<std::vector<std::byte>, Error> Serialize() const;


        uint8_t addArchiveEntry(const std::string& filename, ArchiveLoadType type);


        void registerArchive(
            uint8_t archiveIndex,
            const std::vector<archive::ArchiveEntryInfo>& builtEntries
        );

        FileEntry* findFile(const std::string& name);

    private:
        std::vector<std::byte> SerializeInternal() const;
    };
}