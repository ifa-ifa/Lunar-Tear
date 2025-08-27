#include "replicant/bxon/archive_param.h"
#include "common.h"
#include <cstring>
#include <set>
#include <map>
#include <iostream>
#include <numeric>

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
        uint32_t PackSerialisedSize;
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
    struct RawBxonHeader { char magic[4]; uint32_t version; uint32_t projectID; uint32_t offsetToAssetTypeName; uint32_t offsetToAssetTypeData; };
#pragma pack(pop)

    uint32_t fnv1_32(const std::string& s) {
        uint32_t hash = 0x811c9dc5;
        for (char c : s) {
            hash *= 0x01000193;
            hash ^= static_cast<uint8_t>(c);
        }
        return hash;
    }

    size_t align_to(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
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

    void parseArchiveParameters(ArchiveParameters& asset, const char* buffer, size_t size, const char* asset_base);

    FileEntry* ArchiveParameters::findFile(const std::string& key) {
        for (auto& entry : m_file_entries) {
            if (entry.filePath == key) {
                return &entry;
            }
        }
        return nullptr;
    }

    uint8_t ArchiveParameters::addArchive(std::string filename, ArchiveLoadType loadType) {
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

    void parseArchiveParameters(ArchiveParameters& asset, const char* buffer, size_t size, const char* asset_base) {
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
            entry.PackSerialisedSize = raw_entry.PackSerialisedSize;
            entry.assetsDataSize = raw_entry.assetsDataSize;
            entry.archiveIndex = raw_entry.archiveIndex;
            entry.flags = raw_entry.flags;
            asset.getFileEntries().push_back(std::move(entry));
        }
    }
    ArchiveParameters ArchiveParameters::FromArcEntries(
        const std::vector<archive::ArcWriter::Entry>& entries,
        uint8_t archive_index,
        const std::string& archive_filename,
        ArchiveLoadType load_type)
    {
        ArchiveParameters param;
        param.addArchive(archive_filename, load_type);

        const uint32_t offsetScale = param.getArchiveEntries()[archive_index].offsetScale;

        for (const auto& entry : entries) {
            FileEntry fe;
            fe.filePath = entry.key;
            fe.arcOffset = entry.offset >> offsetScale;
            fe.compressedSize = entry.compressed_size;
            fe.PackSerialisedSize = entry.PackSerialisedSize;
            fe.assetsDataSize = entry.assetsDataSize;
            fe.archiveIndex = archive_index;
            param.getFileEntries().push_back(fe);
        }
        return param;
    }

    std::expected<std::vector<char>, BxonError> ArchiveParameters::build(
        uint32_t version,
        uint32_t projectID) const
    {
        const std::string asset_type_name = "tpArchiveFileParam";
        constexpr size_t ALIGNMENT = 16;

        std::set<std::string> asset_string_set;
        for (const auto& arc : m_archive_entries) { asset_string_set.insert(arc.filename); }
        for (const auto& file : m_file_entries) { asset_string_set.insert(file.filePath); }
        const std::vector<std::string> asset_string_pool(asset_string_set.begin(), asset_string_set.end());
        size_t asset_string_pool_size = 0;
        for (const auto& s : asset_string_pool) { asset_string_pool_size += s.length() + 1; }

        const uint32_t header_start = 0;
        const uint32_t asset_type_name_start = align_to(header_start + sizeof(RawBxonHeader), ALIGNMENT);
        const uint32_t asset_param_start = align_to(asset_type_name_start + asset_type_name.length() + 1, ALIGNMENT);
        const uint32_t archive_array_start = asset_param_start + sizeof(RawArchiveFileParam);
        const uint32_t file_table_start = archive_array_start + (sizeof(RawArchiveEntry) * m_archive_entries.size());
        const uint32_t asset_string_pool_start = file_table_start + (sizeof(RawFileEntry) * m_file_entries.size());
        const size_t total_size = asset_string_pool_start + asset_string_pool_size;
        std::vector<char> buffer(total_size, 0);

        std::map<std::string, uint32_t> asset_string_absolute_offsets;
        uint32_t current_string_offset = asset_string_pool_start;
        for (const auto& s : asset_string_pool) {
            asset_string_absolute_offsets[s] = current_string_offset;
            current_string_offset += s.length() + 1;
        }

        auto* header = reinterpret_cast<RawBxonHeader*>(buffer.data() + header_start);
        memcpy(header->magic, "BXON", 4);
        header->version = version;
        header->projectID = projectID;
        header->offsetToAssetTypeName = asset_type_name_start - (header_start + offsetof(RawBxonHeader, offsetToAssetTypeName));
        header->offsetToAssetTypeData = asset_param_start - (header_start + offsetof(RawBxonHeader, offsetToAssetTypeData));

        memcpy(buffer.data() + asset_type_name_start, asset_type_name.c_str(), asset_type_name.length() + 1);

        auto* asset_header = reinterpret_cast<RawArchiveFileParam*>(buffer.data() + asset_param_start);
        asset_header->numArchives = m_archive_entries.size();
        asset_header->offsetToArray = archive_array_start - (asset_param_start + offsetof(RawArchiveFileParam, offsetToArray));
        asset_header->fileEntryCount = m_file_entries.size();
        asset_header->offsetToFileTable = file_table_start - (asset_param_start + offsetof(RawArchiveFileParam, offsetToFileTable));

        for (size_t i = 0; i < m_archive_entries.size(); ++i) {
            const auto& entry = m_archive_entries[i];
            uint32_t current_arc_entry_start = archive_array_start + (i * sizeof(RawArchiveEntry));
            auto* arc_entry_ptr = reinterpret_cast<RawArchiveEntry*>(buffer.data() + current_arc_entry_start);
            arc_entry_ptr->loadType = entry.loadType;
            arc_entry_ptr->offsetScale = entry.offsetScale;
            arc_entry_ptr->offsetToFilename = asset_string_absolute_offsets.at(entry.filename) - (current_arc_entry_start + offsetof(RawArchiveEntry, offsetToFilename));
        }

        for (size_t i = 0; i < m_file_entries.size(); ++i) {
            const auto& entry = m_file_entries[i];
            uint32_t current_file_entry_start = file_table_start + (i * sizeof(RawFileEntry));
            auto* file_entry_ptr = reinterpret_cast<RawFileEntry*>(buffer.data() + current_file_entry_start);
            file_entry_ptr->pathHash = fnv1_32(entry.filePath);
            file_entry_ptr->arcOffset = entry.arcOffset;
            file_entry_ptr->compressedSize = entry.compressedSize;
            file_entry_ptr->PackSerialisedSize = entry.PackSerialisedSize;
            file_entry_ptr->assetsDataSize = entry.assetsDataSize;
            file_entry_ptr->archiveIndex = entry.archiveIndex;
            file_entry_ptr->flags = entry.flags;
            file_entry_ptr->nameOffset = asset_string_absolute_offsets.at(entry.filePath) - (current_file_entry_start + offsetof(RawFileEntry, nameOffset));
        }

        for (const auto& [str, abs_offset] : asset_string_absolute_offsets) {
            memcpy(buffer.data() + abs_offset, str.c_str(), str.length() + 1);
        }

        return buffer;
    }
}