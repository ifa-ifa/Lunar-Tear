#include "replicant/bxon/archive_param.h"
#include "common.h"
#include <cstring>
#include <set>
#include <iostream>

namespace {
#pragma pack(push, 1)
    struct RawArchiveEntry {
        uint32_t offsetToFilename;
        uint32_t offsetScale;
        replicant::bxon::ArchiveLoadType loadType;
        uint8_t  padding[3];
    };
    struct RawFileEntry {
        uint32_t pathHash;
        uint32_t nameOffset;
        uint32_t arcOffset;
        uint32_t compressedSize;
        uint32_t everythingExceptAssetsDataSize;
        uint32_t assetsDataSize;
        uint8_t  archiveIndex;
        uint8_t  flags;
        uint8_t  padding[2];
    };
    struct RawArchiveFileParam {
        uint32_t numArchives;
        uint32_t offsetToArray;
        uint32_t fileEntryCount;
        uint32_t offsetToFileTable;
    };
#pragma pack(pop)
} 

namespace replicant::bxon::detail {
    std::string ReadStringFromOffset(const char* buffer_base, size_t buffer_size, const void* field_ptr, uint32_t relative_offset) {
        if (!relative_offset || !buffer_base || !field_ptr) return "";
        uintptr_t field_address = reinterpret_cast<uintptr_t>(field_ptr);
        uintptr_t buffer_address = reinterpret_cast<uintptr_t>(buffer_base);
        uintptr_t field_offset_from_base = field_address - buffer_address;
        size_t absolute_offset = field_offset_from_base + relative_offset;
        if (absolute_offset >= buffer_size) return "";
        size_t max_len = buffer_size - absolute_offset;
        size_t actual_len = strnlen(buffer_base + absolute_offset, max_len);
        return std::string(buffer_base + absolute_offset, actual_len);
    }
} 

namespace replicant::bxon {

    void parseArchiveFileParam(ArchiveFileParam& asset, const char* buffer, size_t size, const char* asset_base); 

    FileEntry* ArchiveFileParam::findFileEntryMutable(const std::string& key) {
        for (auto& entry : m_file_entries) {
            if (entry.filePath == key) {
                return &entry;
            }
        }
        return nullptr;
    }

    uint8_t ArchiveFileParam::addArchive(std::string filename, ArchiveLoadType loadType) {
        for (size_t i = 0; i < m_archive_entries.size(); ++i) {
            if (m_archive_entries[i].filename == filename) {
                m_archive_entries[i].loadType = loadType; 
                return static_cast<uint8_t>(i);
            }
        }
        ArchiveEntry new_archive;
        new_archive.filename = std::move(filename);
        new_archive.loadType = loadType;
        m_archive_entries.push_back(new_archive);
        return static_cast<uint8_t>(m_archive_entries.size() - 1);
    }

    void parseArchiveFileParam(ArchiveFileParam& asset, const char* buffer, size_t size, const char* asset_base) {
        const auto* raw_param_header = reinterpret_cast<const RawArchiveFileParam*>(asset_base);

        const uint32_t archive_array_field_abs_pos = (uintptr_t)&raw_param_header->offsetToArray - (uintptr_t)buffer;
        const uint32_t archive_array_abs_pos = archive_array_field_abs_pos + raw_param_header->offsetToArray;

        const uint32_t file_table_field_abs_pos = (uintptr_t)&raw_param_header->offsetToFileTable - (uintptr_t)buffer;
        const uint32_t file_table_abs_pos = file_table_field_abs_pos + raw_param_header->offsetToFileTable;

        if (archive_array_abs_pos >= size || file_table_abs_pos >= size) {
            return;
        }

        const auto* raw_archive_entries = reinterpret_cast<const RawArchiveEntry*>(buffer + archive_array_abs_pos);
        for (uint32_t i = 0; i < raw_param_header->numArchives; ++i) {
            const auto& raw_entry = raw_archive_entries[i];
            ArchiveEntry entry;
            entry.filename = detail::ReadStringFromOffset(buffer, size, &raw_entry.offsetToFilename, raw_entry.offsetToFilename);
            entry.loadType = raw_entry.loadType;
            entry.offsetScale = raw_entry.offsetScale;
            asset.getArchiveEntries().push_back(std::move(entry));
        }

        const auto* raw_file_entries = reinterpret_cast<const RawFileEntry*>(buffer + file_table_abs_pos);
        for (uint32_t i = 0; i < raw_param_header->fileEntryCount; ++i) {
            const auto& raw_entry = raw_file_entries[i];
            FileEntry entry;
            entry.pathHash = raw_entry.pathHash;
            entry.filePath = detail::ReadStringFromOffset(buffer, size, &raw_entry.nameOffset, raw_entry.nameOffset);
            entry.arcOffset = raw_entry.arcOffset;
            entry.compressedSize = raw_entry.compressedSize;
            entry.everythingExceptAssetsDataSize = raw_entry.everythingExceptAssetsDataSize;
            entry.assetsDataSize = raw_entry.assetsDataSize;
            entry.archiveIndex = raw_entry.archiveIndex;
            entry.flags = raw_entry.flags;
            asset.getFileEntries().push_back(std::move(entry));
        }
    }
} 