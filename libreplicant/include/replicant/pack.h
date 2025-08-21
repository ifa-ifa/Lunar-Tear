#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <expected>
#include "replicant/bxon.h"

namespace replicant::pack {

    enum class PackErrorCode {
        Success,
        InvalidMagic,
        BufferTooSmall,
        SizeMismatch,
        ParseError,
        BuildError,
        EntryNotFound
    };

    struct PackError {
        PackErrorCode code;
        std::string message;
    };

    struct PackFileEntry {
        std::string name;
        std::vector<char> serialized_data;
   
        std::vector<char> resource_data;
        uint32_t assetsDataHash = 0;
    };

    class PackFile {
    public:
        std::expected<void, PackError> loadFromMemory(const char* buffer, size_t size);
        std::expected<std::vector<char>, PackError> build();
        std::expected<void, PackError> patchFile(const std::string& entry_name, const std::vector<char>& new_dds_data);

        PackFileEntry* findFileEntryMutable(const std::string& name);
        const std::vector<PackFileEntry>& getFileEntries() const { return m_file_entries; }

        uint32_t getSerializedSize() const { return m_serialized_size; }
        uint32_t getResourceSize() const { return m_resource_size; }
        uint32_t getTotalSize() const { return m_total_size; }

    private:
        uint32_t m_version = 0;
        uint32_t m_total_size = 0;
        uint32_t m_serialized_size = 0;
        uint32_t m_resource_size = 0;
        std::vector<PackFileEntry> m_file_entries;
    };
}